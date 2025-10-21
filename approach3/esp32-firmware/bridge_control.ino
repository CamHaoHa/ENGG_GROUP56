
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

// ==================== WiFi Configuration ====================
const char* ssid = "ESP32_Bridge";
const char* ap_password = "12345678";

// ==================== Pin Definitions ====================
// Traffic Lights
const int TRAFFIC_A_RED = 32;
const int TRAFFIC_A_YELLOW = 14;
const int TRAFFIC_A_GREEN = 27;

// Boat Lights
const int BOAT_B_RED = 23;
const int BOAT_B_YELLOW = 22;
const int BOAT_B_GREEN = 21;

// Motor Control
const int MOTOR_DIRECTION_PIN = 13;
const int MOTOR_SPEED_PIN = 12;

// Boom Gate Servos
const int SERVO_L_PIN = 18;
const int SERVO_R_PIN = 19;

// Ultrasonic Sensors
const int ULTRASONIC1_TRIG = 15;
const int ULTRASONIC1_ECHO = 2;
const int ULTRASONIC2_TRIG = 4;
const int ULTRASONIC2_ECHO = 5;

// Limit Switches (pull-up)
const int LIMIT_SWITCH_TOP = 25;
const int LIMIT_SWITCH_BOTTOM = 33;

// Speaker/Buzzer
const int SPEAKER_PIN = 26;

// LED Indicator
const int LED_PIN = 26;

// ==================== Constants ====================
const int SERVO_UP_ANGLE = 90;      // Traffic passes
const int SERVO_DOWN_ANGLE = 0;     // Traffic blocked
const int MOTOR_SPEED = 255;
const int BOAT_DETECTION_DISTANCE = 100;  // cm

// ==================== State Machine Enums ====================
enum State {
  STATE0_IDLE = 0,              // Bridge closed, traffic flowing
  STATE1_BOAT_DETECTED = 1,     // Boat detected, yellow warning
  STATE2_CLEARING_TRAFFIC = 2,  // Red light, clearing traffic
  STATE2B_TRAFFIC_CLEAR = 3,    // Traffic cleared confirmation
  STATE3_OPENING_BRIDGE = 4,    // Bridge opening
  STATE4_BRIDGE_FULLY_OPEN = 5, // Bridge fully open, yellow to boats
  STATE5_BRIDGE_OPEN = 6,       // Bridge open, boats passing
  STATE6_STOPPING_BOATS = 7,    // Yellow warning to boats
  STATE7_CLOSING_BRIDGE = 8,    // Bridge closing
  STATE8_BRIDGE_CLOSED = 9      // Bridge closed, preparing traffic
};

enum OverrideStep {
  OVERRIDE_READY = 0,
  OVERRIDE_CLEARING_TRAFFIC = 1,
  OVERRIDE_TRAFFIC_CLEAR = 2,
  OVERRIDE_OPENING = 3,
  OVERRIDE_OPEN = 4,
  OVERRIDE_CLOSING = 5
};

// ==================== Global Variables ====================
State currentState = STATE0_IDLE;
OverrideStep overrideStep = OVERRIDE_READY;

unsigned long stateStartTime = 0;
unsigned long overrideStepStartTime = 0;
unsigned long state5EntryTime = 0;
unsigned long lastBoatDetectionTime = 0;
unsigned long lastBeepTime = 0;

bool bridgeOpen = false;
bool stateActionsLogged = false;
bool motorActionLogged = false;
bool manualOverrideActive = false;
bool speakerState = false;
bool speakerManualState = false;

// Servo objects
Servo servoL;
Servo servoR;

// State durations (milliseconds)
const unsigned long DELAY_STATE1 = 3000;   // 3s boat detected warning
const unsigned long DELAY_STATE2 = 5000;   // 5s clearing traffic
const unsigned long DELAY_STATE2B = 3000;  // 3s traffic clear confirmation
const unsigned long DELAY_STATE4 = 3000;   // 3s yellow warning to boats
const unsigned long DELAY_STATE6 = 3000;   // 3s stopping boats
const unsigned long DELAY_STATE8 = 2000;   // 2s preparing traffic

const unsigned long BOAT_CLEAR_DELAY = 3000;      // 3s no boat
const unsigned long STATE5_MAX_DURATION = 10000;  // 10s max wait
const unsigned long BEEP_INTERVAL = 500;          // 500ms beep pattern

// WebServer
WebServer server(80);

// Authentication
const String adminUsername = "admin";
const String adminPassword = "admin";
String authToken = "";
const String REACT_APP_URL = "http://192.168.4.2:3000";

// ==================== Utility Functions ====================
String generateToken() {
  return "token_" + String(random(100000, 999999));
}

// Ultrasonic sensor reading
long readUltrasonic(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);
  long distance = duration * 0.034 / 2;

  return distance;
}

bool isBoatDetected() {
  long distance1 = readUltrasonic(ULTRASONIC1_TRIG, ULTRASONIC1_ECHO);
  long distance2 = readUltrasonic(ULTRASONIC2_TRIG, ULTRASONIC2_ECHO);

  bool detected = (distance1 > 0 && distance1 < BOAT_DETECTION_DISTANCE) ||
                  (distance2 > 0 && distance2 < BOAT_DETECTION_DISTANCE);

  if (detected) {
    lastBoatDetectionTime = millis();
  }

  return detected;
}

bool hasBoatCleared() {
  if (!isBoatDetected()) {
    return (millis() - lastBoatDetectionTime) > BOAT_CLEAR_DELAY;
  }
  return false;
}

bool isBridgeFullyOpen() {
  return digitalRead(LIMIT_SWITCH_TOP) == LOW;
}

bool isBridgeFullyClosed() {
  return digitalRead(LIMIT_SWITCH_BOTTOM) == LOW;
}

// ==================== Hardware Control Functions ====================
void resetLights() {
  digitalWrite(TRAFFIC_A_RED, LOW);
  digitalWrite(TRAFFIC_A_YELLOW, LOW);
  digitalWrite(TRAFFIC_A_GREEN, LOW);
  digitalWrite(BOAT_B_RED, LOW);
  digitalWrite(BOAT_B_YELLOW, LOW);
  digitalWrite(BOAT_B_GREEN, LOW);
}

void motorStop() {
  analogWrite(MOTOR_SPEED_PIN, 0);
  if (!motorActionLogged) {
    Serial.println("Motor: STOPPED");
    motorActionLogged = true;
  }
}

void motorOpen() {
  if (!isBridgeFullyOpen()) {
    digitalWrite(MOTOR_DIRECTION_PIN, HIGH);
    analogWrite(MOTOR_SPEED_PIN, MOTOR_SPEED);
    if (!motorActionLogged) {
      Serial.println("Motor: OPENING bridge");
      motorActionLogged = true;
    }
  } else {
    motorStop();
  }
}

void motorClose() {
  if (!isBridgeFullyClosed()) {
    digitalWrite(MOTOR_DIRECTION_PIN, LOW);
    analogWrite(MOTOR_SPEED_PIN, MOTOR_SPEED);
    if (!motorActionLogged) {
      Serial.println("Motor: CLOSING bridge");
      motorActionLogged = true;
    }
  } else {
    motorStop();
  }
}

void boomGateUp() {
  if (!servoL.attached()) servoL.attach(SERVO_L_PIN);
  if (!servoR.attached()) servoR.attach(SERVO_R_PIN);
  delay(50);
  servoL.write(SERVO_UP_ANGLE);
  servoR.write(SERVO_UP_ANGLE);
  if (!stateActionsLogged) {
    Serial.println("Boom Gates: UP (traffic allowed)");
  }
}

void boomGateDown() {
  if (!servoL.attached()) servoL.attach(SERVO_L_PIN);
  if (!servoR.attached()) servoR.attach(SERVO_R_PIN);
  delay(50);
  servoL.write(SERVO_DOWN_ANGLE);
  servoR.write(SERVO_DOWN_ANGLE);
  if (!stateActionsLogged) {
    Serial.println("Boom Gates: DOWN (traffic blocked)");
  }
}

void boomGateHold() {
  servoL.detach();
  servoR.detach();
}

void speakerBeep(bool active) {
  if (active) {
    unsigned long currentTime = millis();
    if (currentTime - lastBeepTime >= BEEP_INTERVAL) {
      speakerState = !speakerState;
      digitalWrite(SPEAKER_PIN, speakerState ? HIGH : LOW);
      lastBeepTime = currentTime;
    }
  } else {
    digitalWrite(SPEAKER_PIN, LOW);
    speakerState = false;
  }
}

// ==================== State Management ====================
void setLightsForState(State state) {
  resetLights();

  switch (state) {
    case STATE0_IDLE:
      digitalWrite(TRAFFIC_A_GREEN, HIGH);
      digitalWrite(BOAT_B_RED, HIGH);
      boomGateUp();
      speakerBeep(false);
      break;

    case STATE1_BOAT_DETECTED:
      digitalWrite(TRAFFIC_A_YELLOW, HIGH);
      digitalWrite(BOAT_B_RED, HIGH);
      boomGateUp();
      speakerBeep(false);
      break;

    case STATE2_CLEARING_TRAFFIC:
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(BOAT_B_RED, HIGH);
      boomGateUp();
      // Speaker handled in loop
      break;

    case STATE2B_TRAFFIC_CLEAR:
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(BOAT_B_RED, HIGH);
      boomGateDown();
      speakerBeep(false);
      break;

    case STATE3_OPENING_BRIDGE:
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(BOAT_B_RED, HIGH);
      boomGateDown();
      speakerBeep(false);
      break;

    case STATE4_BRIDGE_FULLY_OPEN:
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(BOAT_B_YELLOW, HIGH);
      boomGateDown();
      speakerBeep(false);
      break;

    case STATE5_BRIDGE_OPEN:
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(BOAT_B_GREEN, HIGH);
      boomGateDown();
      speakerBeep(false);
      break;

    case STATE6_STOPPING_BOATS:
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(BOAT_B_YELLOW, HIGH);
      boomGateDown();
      speakerBeep(false);
      break;

    case STATE7_CLOSING_BRIDGE:
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(BOAT_B_RED, HIGH);
      boomGateDown();
      speakerBeep(false);
      break;

    case STATE8_BRIDGE_CLOSED:
      digitalWrite(TRAFFIC_A_YELLOW, HIGH);
      digitalWrite(BOAT_B_RED, HIGH);
      boomGateUp();
      speakerBeep(false);
      break;
  }

  boomGateHold();

  if (!stateActionsLogged) {
    Serial.print("State configured: ");
    Serial.println(state);
    stateActionsLogged = true;
  }
}

void setLightsForOverride() {
  resetLights();
  if (bridgeOpen) {
    digitalWrite(TRAFFIC_A_RED, HIGH);
    digitalWrite(BOAT_B_YELLOW, HIGH);
    boomGateDown();
  } else {
    digitalWrite(TRAFFIC_A_RED, HIGH);
    digitalWrite(BOAT_B_RED, HIGH);
    boomGateUp();
  }
  boomGateHold();
  speakerBeep(false);
  Serial.println("Override mode lights configured");
}

// ==================== Manual Control Functions ====================
void setManualTrafficLight(String light, String color) {
  if (light == "A") {
    digitalWrite(TRAFFIC_A_RED, color == "red" ? HIGH : LOW);
    digitalWrite(TRAFFIC_A_YELLOW, color == "yellow" ? HIGH : LOW);
    digitalWrite(TRAFFIC_A_GREEN, color == "green" ? HIGH : LOW);
  } else if (light == "B") {
    digitalWrite(BOAT_B_RED, color == "red" ? HIGH : LOW);
    digitalWrite(BOAT_B_YELLOW, color == "yellow" ? HIGH : LOW);
    digitalWrite(BOAT_B_GREEN, color == "green" ? HIGH : LOW);
  }
  Serial.println("Manual light: " + light + " = " + color);
}

void manualBoomGateControl(String action) {
  if (action == "up") {
    boomGateUp();
  } else if (action == "down") {
    boomGateDown();
  }
  boomGateHold();
  Serial.println("Manual boom gate: " + action);
}

void manualSpeakerControl() {
  speakerManualState = !speakerManualState;
  digitalWrite(SPEAKER_PIN, speakerManualState ? HIGH : LOW);
  Serial.println("Manual speaker: " + String(speakerManualState ? "ON" : "OFF"));
}

void startManualBridgeOpening() {
  Serial.println("Manual Override: Starting safe bridge opening");

  overrideStep = OVERRIDE_CLEARING_TRAFFIC;
  overrideStepStartTime = millis();

  digitalWrite(TRAFFIC_A_RED, HIGH);
  digitalWrite(TRAFFIC_A_YELLOW, LOW);
  digitalWrite(TRAFFIC_A_GREEN, LOW);
  digitalWrite(BOAT_B_RED, HIGH);

  boomGateUp();
  boomGateHold();

  Serial.println("Override Step 1: Clearing traffic (5s)");
}

void startManualBridgeClosing() {
  Serial.println("Manual Override: Starting safe bridge closing");

  overrideStep = OVERRIDE_CLOSING;
  overrideStepStartTime = millis();

  digitalWrite(BOAT_B_RED, LOW);
  digitalWrite(BOAT_B_YELLOW, HIGH);
  digitalWrite(BOAT_B_GREEN, LOW);

  Serial.println("Override: Closing bridge");
  motorClose();
}

// ==================== Web Server Handlers ====================
void addCorsHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type, x-auth-token");
}

void handleOptions() {
  Serial.println("OPTIONS request");
  addCorsHeaders();
  server.send(204);
}

void handleLogin() {
  Serial.println("POST /api/login");
  addCorsHeaders();

  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    DynamicJsonDocument doc(512);

    if (deserializeJson(doc, body)) {
      server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
      return;
    }

    String username = doc["username"];
    String password = doc["password"];

    if (username == adminUsername && password == adminPassword) {
      authToken = generateToken();
      Serial.println("Login SUCCESS. Token: " + authToken);

      DynamicJsonDocument resp(256);
      resp["success"] = true;
      resp["token"] = authToken;
      String json;
      serializeJson(resp, json);
      server.send(200, "application/json", json);
    } else {
      Serial.println("Login FAILED");
      server.send(401, "application/json", "{\"success\":false,\"message\":\"Invalid credentials\"}");
    }
  } else {
    server.send(400, "application/json", "{\"error\":\"No body\"}");
  }
}

void handleState() {
  addCorsHeaders();

  String token = server.header("x-auth-token");
  if (token == "") token = server.arg("token");

  if (token != authToken || authToken == "") {
    Serial.println("State request UNAUTHORIZED");
    server.send(401, "application/json", "{\"error\":\"Unauthorized\"}");
    return;
  }

  DynamicJsonDocument doc(1024);
  doc["currentState"] = currentState;
  doc["bridgeState"] = bridgeOpen;
  doc["manualOverride"] = manualOverrideActive;
  doc["overrideStep"] = overrideStep;
  doc["redLedA"] = digitalRead(TRAFFIC_A_RED);
  doc["yellowLedA"] = digitalRead(TRAFFIC_A_YELLOW);
  doc["greenLedA"] = digitalRead(TRAFFIC_A_GREEN);
  doc["redLedB"] = digitalRead(BOAT_B_RED);
  doc["yellowLedB"] = digitalRead(BOAT_B_YELLOW);
  doc["greenLedB"] = digitalRead(BOAT_B_GREEN);
  doc["boatDetected"] = isBoatDetected();
  doc["limitTop"] = isBridgeFullyOpen();
  doc["limitBottom"] = isBridgeFullyClosed();

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void handleCommand() {
  Serial.println("POST /api/command");
  addCorsHeaders();

  String token = server.header("x-auth-token");
  if (token == "") token = server.arg("token");

  if (token != authToken || authToken == "") {
    Serial.println("Command UNAUTHORIZED");
    server.send(401, "application/json", "{\"error\":\"Unauthorized\"}");
    return;
  }

  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"No body\"}");
    return;
  }

  String body = server.arg("plain");
  Serial.println("Command body: " + body);

  DynamicJsonDocument doc(512);
  if (deserializeJson(doc, body)) {
    Serial.println("JSON parse ERROR");
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  String action = doc["action"];
  Serial.println("Action: " + action);

  // Enable Override
  if (action == "enableOverride") {
    Serial.println("Enabling Manual Override");
    manualOverrideActive = true;
    overrideStep = OVERRIDE_READY;
    motorStop();
    setLightsForOverride();
    server.send(200, "application/json", "{\"success\":true}");
    return;
  }

  // Disable Override
  if (action == "disableOverride") {
    Serial.println("Disabling Manual Override");
    manualOverrideActive = false;
    overrideStep = OVERRIDE_READY;
    speakerBeep(false);

    if (bridgeOpen) {
      currentState = STATE5_BRIDGE_OPEN;
    } else {
      currentState = STATE0_IDLE;
    }

    stateActionsLogged = false;
    motorActionLogged = false;
    setLightsForState(currentState);
    stateStartTime = millis();
    server.send(200, "application/json", "{\"success\":true}");
    return;
  }

  // Manual Traffic Light Control
  if (action == "setTrafficLight") {
    if (manualOverrideActive) {
      String light = doc["light"];
      String color = doc["color"];
      setManualTrafficLight(light, color);
      server.send(200, "application/json", "{\"success\":true}");
    } else {
      server.send(403, "application/json", "{\"error\":\"Override required\"}");
    }
    return;
  }

  // Manual Boom Gate Control
  if (action == "boomGateControl") {
    if (manualOverrideActive) {
      String gateAction = doc["action"];
      manualBoomGateControl(gateAction);
      server.send(200, "application/json", "{\"success\":true}");
    } else {
      server.send(403, "application/json", "{\"error\":\"Override required\"}");
    }
    return;
  }

  // Manual Speaker Control
  if (action == "speakerControl") {
    if (manualOverrideActive) {
      manualSpeakerControl();
      server.send(200, "application/json", "{\"success\":true}");
    } else {
      server.send(403, "application/json", "{\"error\":\"Override required\"}");
    }
    return;
  }

  // Manual Bridge Open
  if (action == "manualOpen") {
    if (manualOverrideActive) {
      startManualBridgeOpening();
      server.send(200, "application/json", "{\"success\":true}");
    } else {
      server.send(403, "application/json", "{\"error\":\"Override required\"}");
    }
    return;
  }

  // Manual Bridge Close
  if (action == "manualClose") {
    if (manualOverrideActive && bridgeOpen) {
      startManualBridgeClosing();
      server.send(200, "application/json", "{\"success\":true}");
    } else {
      server.send(403, "application/json", "{\"error\":\"Override required or bridge not open\"}");
    }
    return;
  }

  // Auto Mode Open
  if (action == "open") {
    if (!manualOverrideActive && currentState == STATE0_IDLE) {
      Serial.println("Auto: Starting open sequence");
      currentState = STATE1_BOAT_DETECTED;
      stateActionsLogged = false;
      motorActionLogged = false;
      setLightsForState(currentState);
      stateStartTime = millis();
    }
    server.send(200, "application/json", "{\"success\":true}");
    return;
  }

  // Auto Mode Close
  if (action == "close") {
    if (!manualOverrideActive && bridgeOpen && currentState == STATE5_BRIDGE_OPEN) {
      Serial.println("Auto: Starting close sequence");
      currentState = STATE6_STOPPING_BOATS;
      stateActionsLogged = false;
      motorActionLogged = false;
      setLightsForState(currentState);
      stateStartTime = millis();
    }
    server.send(200, "application/json", "{\"success\":true}");
    return;
  }

  // Clear/Reset
  if (action == "clear") {
    Serial.println("CLEAR: Resetting to IDLE");
    manualOverrideActive = false;
    overrideStep = OVERRIDE_READY;
    currentState = STATE0_IDLE;
    stateActionsLogged = false;
    motorActionLogged = false;
    setLightsForState(currentState);
    stateStartTime = millis();
    bridgeOpen = false;
    motorStop();
    speakerBeep(false);
    server.send(200, "application/json", "{\"success\":true}");
    return;
  }

  Serial.println("Unknown action: " + action);
  server.send(400, "application/json", "{\"error\":\"Unknown action\"}");
}

void handleLogout() {
  Serial.println("GET /api/logout");
  addCorsHeaders();

  String token = server.header("x-auth-token");
  if (token == "") token = server.arg("token");

  if (token != authToken || authToken == "") {
    server.send(401, "application/json", "{\"error\":\"Unauthorized\"}");
    return;
  }

  authToken = "";
  Serial.println("Logout SUCCESS");
  server.send(200, "application/json", "{\"success\":true}");
}

void handleRoot() {
  addCorsHeaders();
  server.sendHeader("Location", REACT_APP_URL);
  server.send(302, "text/plain", "Redirecting...");
}

// ==================== Setup ====================
void setup() {
  Serial.begin(115200);
  Serial.println("\n\n=== ESP32 Bridge Control System ===");
  Serial.println("Version: 2.0 - Enhanced with Manual Override");

  randomSeed(analogRead(34));

  // Initialize all pins
  pinMode(TRAFFIC_A_RED, OUTPUT);
  pinMode(TRAFFIC_A_YELLOW, OUTPUT);
  pinMode(TRAFFIC_A_GREEN, OUTPUT);
  pinMode(BOAT_B_RED, OUTPUT);
  pinMode(BOAT_B_YELLOW, OUTPUT);
  pinMode(BOAT_B_GREEN, OUTPUT);
  pinMode(MOTOR_DIRECTION_PIN, OUTPUT);
  pinMode(MOTOR_SPEED_PIN, OUTPUT);
  pinMode(SPEAKER_PIN, OUTPUT);
  pinMode(ULTRASONIC1_TRIG, OUTPUT);
  pinMode(ULTRASONIC1_ECHO, INPUT);
  pinMode(ULTRASONIC2_TRIG, OUTPUT);
  pinMode(ULTRASONIC2_ECHO, INPUT);
  pinMode(LIMIT_SWITCH_TOP, INPUT_PULLUP);
  pinMode(LIMIT_SWITCH_BOTTOM, INPUT_PULLUP);

  // Initialize state
  setLightsForState(currentState);
  motorStop();
  stateStartTime = millis();
  bridgeOpen = false;

  // Initialize servos
  servoL.attach(SERVO_L_PIN);
  servoR.attach(SERVO_R_PIN);
  boomGateUp();

  // Start WiFi AP
  Serial.println("Starting WiFi AP...");
  WiFi.softAP(ssid, ap_password);
  delay(500);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  // Setup web server routes
  server.on("/api/login", HTTP_OPTIONS, handleOptions);
  server.on("/api/login", HTTP_POST, handleLogin);
  server.on("/api/state", HTTP_OPTIONS, handleOptions);
  server.on("/api/state", HTTP_GET, handleState);
  server.on("/api/command", HTTP_OPTIONS, handleOptions);
  server.on("/api/command", HTTP_POST, handleCommand);
  server.on("/api/logout", HTTP_OPTIONS, handleOptions);
  server.on("/api/logout", HTTP_GET, handleLogout);
  server.on("/", HTTP_GET, handleRoot);

  server.begin();
  Serial.println("HTTP server started");
  Serial.println("System ready!\n");
}

// ==================== Main Loop ====================
void loop() {
  server.handleClient();

  // Manual Override Mode
  if (manualOverrideActive) {
    unsigned long currentTime = millis();

    switch (overrideStep) {
      case OVERRIDE_CLEARING_TRAFFIC:
        speakerBeep(true);
        if (currentTime - overrideStepStartTime >= 5000) {
          Serial.println("Override Step 2: Traffic cleared");
          overrideStep = OVERRIDE_TRAFFIC_CLEAR;
          overrideStepStartTime = currentTime;
          boomGateDown();
          boomGateHold();
          speakerBeep(false);
        }
        break;

      case OVERRIDE_TRAFFIC_CLEAR:
        if (currentTime - overrideStepStartTime >= 3000) {
          Serial.println("Override Step 3: Opening bridge");
          overrideStep = OVERRIDE_OPENING;
          overrideStepStartTime = currentTime;
          bridgeOpen = true;
          motorActionLogged = false;
          motorOpen();
        }
        break;

      case OVERRIDE_OPENING:
        if (isBridgeFullyOpen()) {
          Serial.println("Override Step 4: Bridge fully open");
          overrideStep = OVERRIDE_OPEN;
          motorStop();
          digitalWrite(BOAT_B_YELLOW, HIGH);
        }
        break;

      case OVERRIDE_OPEN:
        motorStop();
        break;

      case OVERRIDE_CLOSING:
        motorClose();
        if (isBridgeFullyClosed()) {
          Serial.println("Override: Bridge closed");
          overrideStep = OVERRIDE_READY;
          motorStop();
          bridgeOpen = false;
          digitalWrite(BOAT_B_RED, HIGH);
          digitalWrite(BOAT_B_YELLOW, LOW);
        }
        break;
    }

    // Safety: stop at limits
    if (bridgeOpen && isBridgeFullyOpen()) motorStop();
    if (!bridgeOpen && isBridgeFullyClosed()) motorStop();

    return;
  }

  // ==================== Automatic State Machine ====================
  unsigned long currentTime = millis();

  switch (currentState) {
    case STATE0_IDLE:
      motorStop();
      if (isBoatDetected()) {
        Serial.println(">>> BOAT DETECTED → STATE1");
        currentState = STATE1_BOAT_DETECTED;
        stateActionsLogged = false;
        motorActionLogged = false;
        setLightsForState(currentState);
        stateStartTime = currentTime;
      }
      break;

    case STATE1_BOAT_DETECTED:
      motorStop();
      if (currentTime - stateStartTime >= DELAY_STATE1) {
        Serial.println(">>> STATE1 → STATE2 (Clearing Traffic)");
        currentState = STATE2_CLEARING_TRAFFIC;
        stateActionsLogged = false;
        motorActionLogged = false;
        setLightsForState(currentState);
        stateStartTime = currentTime;
      }
      break;

    case STATE2_CLEARING_TRAFFIC:
      motorStop();
      speakerBeep(true);
      if (currentTime - stateStartTime >= DELAY_STATE2) {
        Serial.println(">>> STATE2 → STATE2B (Traffic Clear)");
        currentState = STATE2B_TRAFFIC_CLEAR;
        stateActionsLogged = false;
        motorActionLogged = false;
        setLightsForState(currentState);
        stateStartTime = currentTime;
      }
      break;

    case STATE2B_TRAFFIC_CLEAR:
      motorStop();
      speakerBeep(false);
      if (currentTime - stateStartTime >= DELAY_STATE2B) {
        Serial.println(">>> STATE2B → STATE3 (Opening Bridge)");
        currentState = STATE3_OPENING_BRIDGE;
        stateActionsLogged = false;
        motorActionLogged = false;
        setLightsForState(currentState);
        stateStartTime = currentTime;
        bridgeOpen = true;
      }
      break;

    case STATE3_OPENING_BRIDGE:
      motorOpen();
      if (isBridgeFullyOpen()) {
        Serial.println(">>> STATE3 → STATE4 (Bridge Fully Open)");
        currentState = STATE4_BRIDGE_FULLY_OPEN;
        stateActionsLogged = false;
        motorActionLogged = false;
        setLightsForState(currentState);
        motorStop();
        stateStartTime = currentTime;
      }
      break;

    case STATE4_BRIDGE_FULLY_OPEN:
      motorStop();
      if (currentTime - stateStartTime >= DELAY_STATE4) {
        Serial.println(">>> STATE4 → STATE5 (Bridge Open)");
        currentState = STATE5_BRIDGE_OPEN;
        stateActionsLogged = false;
        motorActionLogged = false;
        setLightsForState(currentState);
        stateStartTime = currentTime;
        state5EntryTime = currentTime;
      }
      break;

    case STATE5_BRIDGE_OPEN:
      motorStop();
      if (hasBoatCleared()) {
        Serial.println(">>> STATE5 → STATE6 (Boat Cleared)");
        currentState = STATE6_STOPPING_BOATS;
        stateActionsLogged = false;
        motorActionLogged = false;
        setLightsForState(currentState);
        stateStartTime = currentTime;
      } else if (currentTime - state5EntryTime >= STATE5_MAX_DURATION) {
        Serial.println(">>> STATE5 → STATE6 (Timeout)");
        currentState = STATE6_STOPPING_BOATS;
        stateActionsLogged = false;
        motorActionLogged = false;
        setLightsForState(currentState);
        stateStartTime = currentTime;
      }
      break;

    case STATE6_STOPPING_BOATS:
      motorStop();
      if (currentTime - stateStartTime >= DELAY_STATE6) {
        Serial.println(">>> STATE6 → STATE7 (Closing Bridge)");
        currentState = STATE7_CLOSING_BRIDGE;
        stateActionsLogged = false;
        motorActionLogged = false;
        setLightsForState(currentState);
        stateStartTime = currentTime;
        bridgeOpen = false;
      }
      break;

    case STATE7_CLOSING_BRIDGE:
      motorClose();
      if (isBridgeFullyClosed()) {
        Serial.println(">>> STATE7 → STATE8 (Bridge Closed)");
        currentState = STATE8_BRIDGE_CLOSED;
        stateActionsLogged = false;
        motorActionLogged = false;
        setLightsForState(currentState);
        motorStop();
        stateStartTime = currentTime;
      }
      break;

    case STATE8_BRIDGE_CLOSED:
      motorStop();
      if (currentTime - stateStartTime >= DELAY_STATE8) {
        Serial.println(">>> STATE8 → STATE0 (Resuming Traffic)");
        currentState = STATE0_IDLE;
        stateActionsLogged = false;
        motorActionLogged = false;
        setLightsForState(currentState);
        stateStartTime = currentTime;
      }
      break;
  }
}