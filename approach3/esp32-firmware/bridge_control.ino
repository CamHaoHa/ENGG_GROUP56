#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

// WiFi AP credentials
const char* ssid = "group56bridge";
const char* ap_password = "bridgehereweare";

// Pin definitions
const int LED_PIN = 26;
const int TRAFFIC_A_RED = 32;
const int TRAFFIC_A_YELLOW = 14;
const int TRAFFIC_A_GREEN = 27;
const int BOAT_B_RED = 23;
const int BOAT_B_YELLOW = 22;
const int BOAT_B_GREEN = 21;
const int MOTOR_DIRECTION_PIN = 13;
const int MOTOR_SPEED_PIN = 12;
const int SERVO_L_PIN = 18;
const int SERVO_R_PIN = 19;

// Ultrasonic sensor pins
const int ULTRASONIC_1_TRIG = 15;  // D15 - Entry sensor
const int ULTRASONIC_1_ECHO = 2;   // D2
const int ULTRASONIC_2_TRIG = 4;   // D4 - Exit sensor
const int ULTRASONIC_2_ECHO = 16;  // D16

// Limit switch pins (active LOW with pull-up)
const int LIMIT_SWITCH_TOP = 25;    // D25 - Bridge fully open
const int LIMIT_SWITCH_BOTTOM = 33; // D33 - Bridge fully closed

// Ultrasonic sensor settings
const int LARGE_BOAT_MIN_CM = 10;
const int LARGE_BOAT_MAX_CM = 30;
const int MIN_DETECTION_TIME = 500;
const long ULTRASONIC_TIMEOUT = 5000;

// Servo angles
const int SERVO_RAISED_ANGLE = 0;
const int SERVO_LOWERED_ANGLE = 90;

// Motor speed
const int MOTOR_SPEED = 120;  // 0-255 PWM value

// Motor direction control
bool forwardDirection = true;  // true = opening, false = closing
bool prevTopTriggered = false;
bool prevBottomTriggered = false;

// Motor ramping control
int currentMotorSpeed = 0;
unsigned long lastRampTime = 0;
const int RAMP_STEP = 10;
const int RAMP_INTERVAL = 50;  // ms between ramp steps

// State machine enum
enum State {
  STATE0,   // IDLE
  STATE1,   // BOAT DETECTED
  STATE2,   // CLEARING TRAFFIC
  STATE2B,  // TRAFFIC CLEAR CONFIRMATION
  STATE3,   // OPENING BRIDGE
  STATE4,   // BRIDGE FULLY OPEN (YELLOW WARNING)
  STATE5,   // BRIDGE OPEN (WAITING FOR BOATS)
  STATE6,   // STOPPING BOATS
  STATE7,   // CLOSING BRIDGE
  STATE8    // BRIDGE FULLY CLOSED (PREPARING TRAFFIC)
};

// State variables
State currentState = STATE0;
unsigned long stateStartTime = 0;
bool bridgeOpen = false;
bool stateActionsLogged = false;
bool motorActionLogged = false;
bool manualOverrideActive = false;

// Manual override variables
bool manualOperationInProgress = false;
unsigned long manualOperationStart = 0;
String currentManualAction = "";

// Ultrasonic sensor variables
unsigned long lastDetectionTime = 0;
bool boatDetectedSensor1 = false;
bool boatDetectedSensor2 = false;
bool boatCurrentlyDetected = false;
bool boatInPassage = false;
bool entryCleared = false;
unsigned long lastExitDetectionTime = 0;
unsigned long lastReverseDetectionTime = 0;
unsigned long lastUltrasonicCheck = 0;
const unsigned long ULTRASONIC_CHECK_INTERVAL = 200;

// Servo objects
Servo servoL;
Servo servoR;

// State durations
const unsigned long DELAY_STATE1 = 5000;
const unsigned long DELAY_STATE2 = 10000;
const unsigned long DELAY_STATE2B = 5000;
const unsigned long DELAY_STATE3 = 15000; //opening bridge 15s
const unsigned long DELAY_STATE4 = 5000;
const unsigned long DELAY_STATE5 = 15000;
const unsigned long DELAY_STATE6 = 5000;
const unsigned long DELAY_STATE7 = 15000; //closing bridge 15s
const unsigned long DELAY_STATE8 = 30000;

// WebServer
WebServer server(80);

// Authentication
const String adminUsername = "admin";
const String adminPassword = "admin";
String authToken = "";
const String REACT_APP_URL = "http://192.168.4.4:3000";

String generateToken() {
  return "secure_token_" + String(random(100000, 999999));
}

// Check limit switches
bool isBridgeFullyOpen() {
  return digitalRead(LIMIT_SWITCH_TOP) == LOW;
}

bool isBridgeFullyClosed() {
  return digitalRead(LIMIT_SWITCH_BOTTOM) == LOW;
}

void handleLimitSwitches() {
  // Edge-detection logging only — do not auto-reverse direction here.
  bool currentTop = isBridgeFullyOpen();
  bool currentBottom = isBridgeFullyClosed();

  if (currentTop && !prevTopTriggered) {
    Serial.println("🔔 Top limit hit");
  }
  prevTopTriggered = currentTop;

  if (currentBottom && !prevBottomTriggered) {
    Serial.println("🔔 Bottom limit hit");
  }
  prevBottomTriggered = currentBottom;
}

// Read ultrasonic distance
long readUltrasonicDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH, ULTRASONIC_TIMEOUT);
  if (duration == 0) return -1;
  
  long distance = duration * 0.0343 / 2;
  if (distance < 2 || distance > 400) return -1;
  
  return distance;
}

// Simplified reset - FORCE close bridge at SLOWER speed
void performSystemReset() {
  Serial.println("\n🔄 SYSTEM RESET - Forcing bridge to IDLE");

  manualOverrideActive = false;
  manualOperationInProgress = false;

  digitalWrite(TRAFFIC_A_RED, HIGH);
  digitalWrite(TRAFFIC_A_YELLOW, LOW);
  digitalWrite(TRAFFIC_A_GREEN, LOW);
  digitalWrite(BOAT_B_RED, HIGH);
  digitalWrite(BOAT_B_YELLOW, LOW);
  digitalWrite(BOAT_B_GREEN, LOW);

  Serial.println("⬇️  Closing bridge (forced - 15s max, slower speed)...");

  // Set direction to closing
  forwardDirection = false;
  digitalWrite(MOTOR_DIRECTION_PIN, LOW);

  unsigned long resetStart = millis();
  bool limitReached = false;
  
  // Use SLOWER speed for reset (40% of normal)
  int resetSpeed = MOTOR_SPEED * 0.8;

  while ((millis() - resetStart < 15000)) {
    bool currentBottom = isBridgeFullyClosed();

    if (currentBottom) {
      limitReached = true;
      Serial.println(" ✓ Bottom limit reached");
      break;
    }

    // Run motor at slower speed while NOT at bottom
    if (!currentBottom) {
      analogWrite(MOTOR_SPEED_PIN, resetSpeed);
    } else {
      analogWrite(MOTOR_SPEED_PIN, 0);
    }

    if ((millis() - resetStart) % 1000 == 0) {
      Serial.print(".");
    }
    delay(100);
  }

  analogWrite(MOTOR_SPEED_PIN, 0);

  if (!limitReached) {
    Serial.println("\n⚠️  Timeout - check switch wiring");
  }

  // Reset state to safe defaults
  currentState = STATE0;
  bridgeOpen = false;
  boatInPassage = false;
  entryCleared = false;
  stateActionsLogged = false;
  motorActionLogged = false;
  lastDetectionTime = 0;
  lastExitDetectionTime = 0;
  lastReverseDetectionTime = 0;
  currentMotorSpeed = 0;

  digitalWrite(TRAFFIC_A_GREEN, HIGH);
  digitalWrite(TRAFFIC_A_RED, LOW);
  digitalWrite(BOAT_B_RED, HIGH);

  stateStartTime = millis();
  digitalWrite(LED_PIN, LOW);

  Serial.println("✅ Reset complete - IDLE\n");
}

// Boat detection
bool detectBoat() {
  long distance1 = readUltrasonicDistance(ULTRASONIC_1_TRIG, ULTRASONIC_1_ECHO);
  long distance2 = readUltrasonicDistance(ULTRASONIC_2_TRIG, ULTRASONIC_2_ECHO);
  
  boatDetectedSensor1 = (distance1 >= LARGE_BOAT_MIN_CM && distance1 <= LARGE_BOAT_MAX_CM);
  boatDetectedSensor2 = (distance2 >= LARGE_BOAT_MIN_CM && distance2 <= LARGE_BOAT_MAX_CM);
  
  if (boatDetectedSensor1) {
    Serial.print("🚤 ENTRY: ");
    Serial.print(distance1);
    Serial.println(" cm");
  }
  if (boatDetectedSensor2) {
    Serial.print("🚤 EXIT: ");
    Serial.print(distance2);
    Serial.println(" cm");
  }
  
  boatCurrentlyDetected = boatDetectedSensor1 || boatDetectedSensor2;
  return boatCurrentlyDetected;
}

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
  currentMotorSpeed = 0;
  if (!motorActionLogged) {
    Serial.println("🛑 Motor: Stopped");
    motorActionLogged = true;
  }
}

// Non-blocking motor ramp for opening
void motorOpen() {
  forwardDirection = true;
  digitalWrite(MOTOR_DIRECTION_PIN, HIGH);

  bool currentTop = isBridgeFullyOpen();

  if (!currentTop) {
    unsigned long currentTime = millis();
    
    // Ramp speed gradually (non-blocking)
    if (currentMotorSpeed < MOTOR_SPEED && currentTime - lastRampTime >= RAMP_INTERVAL) {
      currentMotorSpeed += RAMP_STEP;
      if (currentMotorSpeed > MOTOR_SPEED) currentMotorSpeed = MOTOR_SPEED;
      analogWrite(MOTOR_SPEED_PIN, currentMotorSpeed);
      lastRampTime = currentTime;
    }
    
    if (!motorActionLogged) {
      Serial.println("⬆️  Motor: Opening");
      motorActionLogged = true;
    }
  } else {
    analogWrite(MOTOR_SPEED_PIN, 0);
    currentMotorSpeed = 0;
    if (!motorActionLogged) {
      Serial.println("⚠️  Motor: Top limit active, stopped");
      motorActionLogged = true;
    }
  }
}

// Non-blocking motor ramp for closing
void motorClose() {
  forwardDirection = false;
  digitalWrite(MOTOR_DIRECTION_PIN, LOW);

  bool currentBottom = isBridgeFullyClosed();

  if (!currentBottom) {
    unsigned long currentTime = millis();
    
    // Ramp speed gradually (non-blocking)
    if (currentMotorSpeed < MOTOR_SPEED && currentTime - lastRampTime >= RAMP_INTERVAL) {
      currentMotorSpeed += RAMP_STEP;
      if (currentMotorSpeed > MOTOR_SPEED) currentMotorSpeed = MOTOR_SPEED;
      analogWrite(MOTOR_SPEED_PIN, currentMotorSpeed);
      lastRampTime = currentTime;
    }
    
    if (!motorActionLogged) {
      Serial.println("⬇️  Motor: Closing");
      motorActionLogged = true;
    }
  } else {
    analogWrite(MOTOR_SPEED_PIN, 0);
    currentMotorSpeed = 0;
    if (!motorActionLogged) {
      Serial.println("⚠️  Motor: Bottom limit active, stopped");
      motorActionLogged = true;
    }
  }
}

void boomGateRaise() {
  if (!servoL.attached()) servoL.attach(SERVO_L_PIN);
  if (!servoR.attached()) servoR.attach(SERVO_R_PIN);
  delay(50);
  servoL.write(SERVO_RAISED_ANGLE);
  servoR.write(SERVO_RAISED_ANGLE);
  if (!stateActionsLogged) {
    Serial.println("⬆️  Boom Gate: Raised");
  }
}

void boomGateLower() {
  if (!servoL.attached()) servoL.attach(SERVO_L_PIN);
  if (!servoR.attached()) servoR.attach(SERVO_R_PIN);
  delay(50);
  servoL.write(SERVO_LOWERED_ANGLE);
  servoR.write(SERVO_LOWERED_ANGLE);
  if (!stateActionsLogged) {
    Serial.println("⬇️  Boom Gate: Lowered");
  }
}

void boomGateHold() {
  servoL.detach();
  servoR.detach();
}

void setLightsForState(State state) {
  resetLights();
  boomGateHold();
  
  switch (state) {
    case STATE0:
      digitalWrite(TRAFFIC_A_GREEN, HIGH);
      digitalWrite(BOAT_B_RED, HIGH);
      boomGateRaise();
      break;
    case STATE1:
      digitalWrite(TRAFFIC_A_YELLOW, HIGH);
      digitalWrite(BOAT_B_RED, HIGH);
      boomGateRaise();
      break;
    case STATE2:
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(BOAT_B_RED, HIGH);
      boomGateRaise();
      break;
    case STATE2B:
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(BOAT_B_RED, HIGH);
      boomGateLower();
      break;
    case STATE3:
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(BOAT_B_RED, HIGH);
      boomGateLower();
      break;
    case STATE4:
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(BOAT_B_YELLOW, HIGH);
      boomGateLower();
      break;
    case STATE5:
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(BOAT_B_GREEN, HIGH);
      boomGateLower();
      break;
    case STATE6:
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(BOAT_B_YELLOW, HIGH);
      boomGateLower();
      break;
    case STATE7:
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(BOAT_B_RED, HIGH);
      boomGateLower();
      break;
    case STATE8:
      digitalWrite(TRAFFIC_A_YELLOW, HIGH);
      digitalWrite(BOAT_B_RED, HIGH);
      boomGateRaise();
      break;
  }
  
  if (!stateActionsLogged) {
    Serial.print("💡 STATE");
    Serial.println(state);
    stateActionsLogged = true;
  }
}

void setLightsForOverride() {
  resetLights();
  if (bridgeOpen) {
    digitalWrite(TRAFFIC_A_RED, HIGH);
    digitalWrite(BOAT_B_YELLOW, HIGH);
    boomGateLower();
  } else {
    digitalWrite(TRAFFIC_A_RED, HIGH);
    digitalWrite(BOAT_B_RED, HIGH);
    boomGateRaise();
  }
}

void addCorsHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type, x-auth-token, X-Auth-Token, X-AUTH-TOKEN");
}

void handleOptions() {
  addCorsHeaders();
  server.send(204);
}

void handleLogin() {
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
      Serial.print("🔑 Login: ");
      Serial.println(authToken);
      
      DynamicJsonDocument resp(256);
      resp["success"] = true;
      resp["token"] = authToken;
      String json;
      serializeJson(resp, json);
      server.send(200, "application/json", json);
    } else {
      server.send(401, "application/json", "{\"success\":false,\"message\":\"Invalid credentials\"}");
    }
  } else {
    server.send(400, "application/json", "{\"error\":\"No body\"}");
  }
}

void handleState() {
  addCorsHeaders();
  
  String token = server.header("x-auth-token");
  if (token == "") token = server.header("X-Auth-Token");
  if (token == "") token = server.header("X-AUTH-TOKEN");
  if (token == "" && server.hasArg("token")) token = server.arg("token");
  
  if (token != authToken || authToken == "") {
    server.send(401, "application/json", "{\"error\":\"Unauthorized\"}");
    return;
  }
  
  DynamicJsonDocument doc(1024);
  doc["currentState"] = currentState;
  doc["bridgeState"] = bridgeOpen;
  doc["manualOverride"] = manualOverrideActive;
  doc["boatDetected"] = boatCurrentlyDetected;
  doc["boatSensor1"] = boatDetectedSensor1;
  doc["boatSensor2"] = boatDetectedSensor2;
  doc["redLedA"] = digitalRead(TRAFFIC_A_RED);
  doc["yellowLedA"] = digitalRead(TRAFFIC_A_YELLOW);
  doc["greenLedA"] = digitalRead(TRAFFIC_A_GREEN);
  doc["redLedB"] = digitalRead(BOAT_B_RED);
  doc["yellowLedB"] = digitalRead(BOAT_B_YELLOW);
  doc["greenLedB"] = digitalRead(BOAT_B_GREEN);
  doc["limitSwitchTop"] = isBridgeFullyOpen();
  doc["limitSwitchBottom"] = isBridgeFullyClosed();
  
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void handleCommand() {
  addCorsHeaders();
  
  String token = server.header("x-auth-token");
  if (token == "") token = server.header("X-Auth-Token");
  if (token == "") token = server.header("X-AUTH-TOKEN");
  if (token == "" && server.hasArg("token")) token = server.arg("token");
  
  if (token != authToken || authToken == "") {
    server.send(401, "application/json", "{\"error\":\"Unauthorized\"}");
    return;
  }
  
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    DynamicJsonDocument doc(512);
    
    if (deserializeJson(doc, body)) {
      server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
      return;
    }
    
    String action = doc["action"];
    Serial.print("📡 Command: ");
    Serial.println(action);
    
    if (action == "enableOverride") {
      manualOverrideActive = true;
      motorStop();
      setLightsForOverride();
      server.send(200, "application/json", "{\"success\":true}");
      return;
    }
    
    if (action == "disableOverride") {
      manualOverrideActive = false;
      manualOperationInProgress = false;
      currentState = bridgeOpen ? STATE5 : STATE0;
      stateActionsLogged = false;
      motorActionLogged = false;
      currentMotorSpeed = 0;
      setLightsForState(currentState);
      stateStartTime = millis();
      server.send(200, "application/json", "{\"success\":true}");
      return;
    }
    
    if (manualOverrideActive) {
      if (action == "trafficRed") {
        digitalWrite(TRAFFIC_A_RED, HIGH);
        digitalWrite(TRAFFIC_A_YELLOW, LOW);
        digitalWrite(TRAFFIC_A_GREEN, LOW);
        server.send(200, "application/json", "{\"success\":true}");
      } else if (action == "trafficYellow") {
        digitalWrite(TRAFFIC_A_RED, LOW);
        digitalWrite(TRAFFIC_A_YELLOW, HIGH);
        digitalWrite(TRAFFIC_A_GREEN, LOW);
        server.send(200, "application/json", "{\"success\":true}");
      } else if (action == "trafficGreen") {
        digitalWrite(TRAFFIC_A_RED, LOW);
        digitalWrite(TRAFFIC_A_YELLOW, LOW);
        digitalWrite(TRAFFIC_A_GREEN, HIGH);
        server.send(200, "application/json", "{\"success\":true}");
      } else if (action == "boatRed") {
        digitalWrite(BOAT_B_RED, HIGH);
        digitalWrite(BOAT_B_YELLOW, LOW);
        digitalWrite(BOAT_B_GREEN, LOW);
        server.send(200, "application/json", "{\"success\":true}");
      } else if (action == "boatYellow") {
        digitalWrite(BOAT_B_RED, LOW);
        digitalWrite(BOAT_B_YELLOW, HIGH);
        digitalWrite(BOAT_B_GREEN, LOW);
        server.send(200, "application/json", "{\"success\":true}");
      } else if (action == "boatGreen") {
        digitalWrite(BOAT_B_RED, LOW);
        digitalWrite(BOAT_B_YELLOW, LOW);
        digitalWrite(BOAT_B_GREEN, HIGH);
        server.send(200, "application/json", "{\"success\":true}");
      } else if (action == "open") {
        bridgeOpen = true;
        motorActionLogged = false;
        currentMotorSpeed = 0;
        setLightsForOverride();
        digitalWrite(LED_PIN, HIGH);
        manualOperationInProgress = true;
        manualOperationStart = millis();
        currentManualAction = "opening";
        server.send(200, "application/json", "{\"success\":true}");
      } else if (action == "close") {
        bridgeOpen = false;
        motorActionLogged = false;
        currentMotorSpeed = 0;
        setLightsForOverride();
        digitalWrite(LED_PIN, LOW);
        manualOperationInProgress = true;
        manualOperationStart = millis();
        currentManualAction = "closing";
        server.send(200, "application/json", "{\"success\":true}");
      } else {
        server.send(400, "application/json", "{\"error\":\"Unknown action\"}");
      }
    } else {
      if (action == "open" && currentState == STATE0) {
        currentState = STATE1;
        boatInPassage = true;
        entryCleared = false;
        stateActionsLogged = false;
        motorActionLogged = false;
        currentMotorSpeed = 0;
        setLightsForState(currentState);
        stateStartTime = millis();
        digitalWrite(LED_PIN, HIGH);
        server.send(200, "application/json", "{\"success\":true}");
      } else if (action == "close" && bridgeOpen && currentState == STATE5) {
        currentState = STATE6;
        boatInPassage = false;
        entryCleared = false;
        stateActionsLogged = false;
        motorActionLogged = false;
        currentMotorSpeed = 0;
        setLightsForState(currentState);
        stateStartTime = millis();
        server.send(200, "application/json", "{\"success\":true}");
      } else if (action == "clear") {
        performSystemReset();
        server.send(200, "application/json", "{\"success\":true}");
      } else {
        server.send(400, "application/json", "{\"error\":\"Invalid state\"}");
      }
    }
  } else {
    server.send(400, "application/json", "{\"error\":\"No body\"}");
  }
}

void handleLogout() {
  addCorsHeaders();
  
  String token = server.header("x-auth-token");
  if (token == "") token = server.header("X-Auth-Token");
  if (token == "") token = server.header("X-AUTH-TOKEN");
  if (token == "" && server.hasArg("token")) token = server.arg("token");
  
  if (token != authToken || authToken == "") {
    server.send(401, "application/json", "{\"error\":\"Unauthorized\"}");
    return;
  }
  
  authToken = "";
  Serial.println("🔓 Logout");
  server.send(200, "application/json", "{\"success\":true}");
}

void handleRoot() {
  addCorsHeaders();
  server.sendHeader("Location", REACT_APP_URL);
  server.send(302, "text/plain", "Redirecting...");
}

void setup() {
  randomSeed(analogRead(34));
  
  pinMode(TRAFFIC_A_RED, OUTPUT);
  pinMode(TRAFFIC_A_YELLOW, OUTPUT);
  pinMode(TRAFFIC_A_GREEN, OUTPUT);
  pinMode(BOAT_B_RED, OUTPUT);
  pinMode(BOAT_B_YELLOW, OUTPUT);
  pinMode(BOAT_B_GREEN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(MOTOR_DIRECTION_PIN, OUTPUT);
  pinMode(MOTOR_SPEED_PIN, OUTPUT);
  pinMode(ULTRASONIC_1_TRIG, OUTPUT);
  pinMode(ULTRASONIC_1_ECHO, INPUT);
  pinMode(ULTRASONIC_2_TRIG, OUTPUT);
  pinMode(ULTRASONIC_2_ECHO, INPUT);
  pinMode(LIMIT_SWITCH_TOP, INPUT_PULLUP);
  pinMode(LIMIT_SWITCH_BOTTOM, INPUT_PULLUP);
  
  Serial.begin(115200);
  Serial.println("\n========================================");
  Serial.println("🌉 ESP32 Bridge Control v3.5");
  Serial.println("========================================");
  
  Serial.println("\n🔧 Limit Switches:");
  Serial.print("  Top (D25): ");
  Serial.println(digitalRead(LIMIT_SWITCH_TOP) == LOW ? "PRESSED" : "RELEASED");
  Serial.print("  Bottom (D33): ");
  Serial.println(digitalRead(LIMIT_SWITCH_BOTTOM) == LOW ? "PRESSED" : "RELEASED");
  
  // Initialize to closed position
  performSystemReset();
  
  setLightsForState(currentState);
  motorStop();
  stateStartTime = millis();
  
  servoL.attach(SERVO_L_PIN);
  servoR.attach(SERVO_R_PIN);
  boomGateRaise();
  
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
  
  Serial.println("\n📡 Sensors:");
  Serial.println("  Entry (10-30cm) → Triggers bridge");
  Serial.println("  Exit (10-30cm) → Confirms passage");
  
  WiFi.softAP(ssid, ap_password);
  Serial.print("\n📶 AP IP: ");
  Serial.println(WiFi.softAPIP());
  
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
  Serial.println("✅ Ready!\n");
}

void loop() {
  server.handleClient();
  
  unsigned long currentTime = millis();
  
  // Handle limit switch edge detection in ALL modes
  handleLimitSwitches();
  
  // Manual operation timeout and motor control
  if (manualOverrideActive && manualOperationInProgress) {
    if (currentManualAction == "opening") {
      motorOpen();
      if (isBridgeFullyOpen() || currentTime - manualOperationStart >= DELAY_STATE3) {
        motorStop();
        manualOperationInProgress = false;
      }
    } else if (currentManualAction == "closing") {
      motorClose();
      if (isBridgeFullyClosed() || currentTime - manualOperationStart >= DELAY_STATE7) {
        motorStop();
        manualOperationInProgress = false;
      }
    }
  }
  
  // Boat detection
  if (currentTime - lastUltrasonicCheck >= ULTRASONIC_CHECK_INTERVAL) {
    lastUltrasonicCheck = currentTime;
    detectBoat();
    
    if (!manualOverrideActive && currentState == STATE0 && boatDetectedSensor1) {
      if (lastDetectionTime == 0) {
        lastDetectionTime = currentTime;
        Serial.println("⚠️  Entry triggered - 500ms...");
      } else if (currentTime - lastDetectionTime >= MIN_DETECTION_TIME) {
        Serial.println("✅ STATE1");
        currentState = STATE1;
        boatInPassage = true;
        entryCleared = false;
        stateActionsLogged = false;
        motorActionLogged = false;
        currentMotorSpeed = 0;
        setLightsForState(currentState);
        stateStartTime = millis();
        digitalWrite(LED_PIN, HIGH);
        lastDetectionTime = 0;
      }
    } else if (!boatDetectedSensor1) {
      lastDetectionTime = 0;
    }
  }
  
  if (manualOverrideActive) return;
  
  // State machine
  switch (currentState) {
    case STATE0:
      motorStop();
      break;
      
    case STATE1:
      motorStop();
      if (currentTime - stateStartTime >= DELAY_STATE1) {
        Serial.println("STATE1 → STATE2");
        currentState = STATE2;
        stateActionsLogged = false;
        motorActionLogged = false;
        currentMotorSpeed = 0;
        setLightsForState(currentState);
        stateStartTime = currentTime;
      }
      break;
      
    case STATE2:
      motorStop();
      if (currentTime - stateStartTime >= DELAY_STATE2) {
        Serial.println("STATE2 → STATE2B");
        currentState = STATE2B;
        stateActionsLogged = false;
        motorActionLogged = false;
        currentMotorSpeed = 0;
        setLightsForState(currentState);
        stateStartTime = currentTime;
      }
      break;
      
    case STATE2B:
      motorStop();
      if (currentTime - stateStartTime >= DELAY_STATE2B) {
        Serial.println("STATE2B → STATE3");
        currentState = STATE3;
        stateActionsLogged = false;
        motorActionLogged = false;
        currentMotorSpeed = 0;
        setLightsForState(currentState);
        stateStartTime = currentTime;
      }
      break;
      
    case STATE3:
      motorOpen();
      if (isBridgeFullyOpen()) {
        Serial.println("STATE3 → STATE4 (limit)");
        currentState = STATE4;
        stateActionsLogged = false;
        motorActionLogged = false;
        setLightsForState(currentState);
        motorStop();
        stateStartTime = currentTime;
        bridgeOpen = true;
      } else if (currentTime - stateStartTime >= DELAY_STATE3) {
        Serial.println("STATE3 → STATE4 (timeout)");
        currentState = STATE4;
        stateActionsLogged = false;
        motorActionLogged = false;
        setLightsForState(currentState);
        motorStop();
        stateStartTime = currentTime;
        bridgeOpen = true;
      }
      break;
      
    case STATE4:
      motorStop();
      if (currentTime - stateStartTime >= DELAY_STATE4) {
        Serial.println("STATE4 → STATE5");
        currentState = STATE5;
        stateActionsLogged = false;
        motorActionLogged = false;
        currentMotorSpeed = 0;
        setLightsForState(currentState);
        stateStartTime = currentTime;
        entryCleared = false;
      }
      break;
      
    case STATE5:
      motorStop();
      
      if (!boatDetectedSensor1 && !entryCleared) {
        entryCleared = true;
        Serial.println("✅ Entry cleared");
      }
      
      // Enforce minimum 10 seconds before checking sensors
      if (currentTime - stateStartTime >= 10000) {
        if (boatDetectedSensor2) {
          if (lastExitDetectionTime == 0) {
            lastExitDetectionTime = currentTime;
            Serial.println("🚤 Exit detecting...");
          } else if (currentTime - lastExitDetectionTime >= MIN_DETECTION_TIME) {
            Serial.println("✅ Boat exited → STATE6");
            boatInPassage = false;
            entryCleared = false;
            currentState = STATE6;
            stateActionsLogged = false;
            motorActionLogged = false;
            currentMotorSpeed = 0;
            setLightsForState(currentState);
            stateStartTime = currentTime;
            lastExitDetectionTime = 0;
            lastReverseDetectionTime = 0;
          }
        } else {
          lastExitDetectionTime = 0;
        }
        
        if (entryCleared && boatDetectedSensor1) {
          if (lastReverseDetectionTime == 0) {
            lastReverseDetectionTime = currentTime;
            Serial.println("⚠️  Reverse...");
          } else if (currentTime - lastReverseDetectionTime >= MIN_DETECTION_TIME) {
            Serial.println("⚠️  Reversed → STATE6");
            boatInPassage = false;
            entryCleared = false;
            currentState = STATE6;
            stateActionsLogged = false;
            motorActionLogged = false;
            currentMotorSpeed = 0;
            setLightsForState(currentState);
            stateStartTime = currentTime;
            lastReverseDetectionTime = 0;
            lastExitDetectionTime = 0;
          }
        } else if (!boatDetectedSensor1) {
          lastReverseDetectionTime = 0;
        }
      }
      
      if (currentTime - stateStartTime >= DELAY_STATE5) {
        Serial.println("⏰ TIMEOUT → STATE6");
        boatInPassage = false;
        entryCleared = false;
        currentState = STATE6;
        stateActionsLogged = false;
        motorActionLogged = false;
        currentMotorSpeed = 0;
        setLightsForState(currentState);
        stateStartTime = currentTime;
        lastExitDetectionTime = 0;
        lastReverseDetectionTime = 0;
      }
      break;
      
    case STATE6:
      motorStop();
      if (currentTime - stateStartTime >= DELAY_STATE6) {
        Serial.println("STATE6 → STATE7");
        currentState = STATE7;
        stateActionsLogged = false;
        motorActionLogged = false;
        currentMotorSpeed = 0;
        setLightsForState(currentState);
        stateStartTime = currentTime;
      }
      break;
      
    case STATE7:
      motorClose();
      if (isBridgeFullyClosed()) {
        Serial.println("STATE7 → STATE8 (limit)");
        currentState = STATE8;
        stateActionsLogged = false;
        motorActionLogged = false;
        setLightsForState(currentState);
        motorStop();
        stateStartTime = currentTime;
        bridgeOpen = false;
      } else if (currentTime - stateStartTime >= DELAY_STATE7) {
        Serial.println("STATE7 → STATE8 (timeout)");
        currentState = STATE8;
        stateActionsLogged = false;
        motorActionLogged = false;
        setLightsForState(currentState);
        motorStop();
        stateStartTime = currentTime;
        bridgeOpen = false;
      }
      break;
      
    case STATE8:
      motorStop();
      if (currentTime - stateStartTime >= DELAY_STATE8) {
        Serial.println("STATE8 → STATE0");
        currentState = STATE0;
        stateActionsLogged = false;
        motorActionLogged = false;
        currentMotorSpeed = 0;
        setLightsForState(currentState);
        stateStartTime = currentTime;
        digitalWrite(LED_PIN, LOW);
      }
      break;
  }
}