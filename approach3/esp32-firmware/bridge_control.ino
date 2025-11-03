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

// Ultrasonic sensor settings - FIXED LOGIC
const int DETECTION_DISTANCE_MIN_CM = 30;  // Minimum detection distance
const int DETECTION_DISTANCE_MAX_CM = 50;  // Maximum detection distance
const int SMALL_BOAT_THRESHOLD_CM = 35;    // Boats CLOSER than this (< 35cm) are small boats
const int MIN_DETECTION_TIME = 500;        // Minimum detection time in ms
const long ULTRASONIC_TIMEOUT = 5000;      // 5ms timeout (was 30ms - too long)

// Servo angles
const int SERVO_RAISED_ANGLE = 0;
const int SERVO_LOWERED_ANGLE = 90;

// Motor speed
const int MOTOR_SPEED = 255;

// Updated enum for states (now 10 states with STATE2B)
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

// Manual override non-blocking variables
bool manualOperationInProgress = false;
unsigned long manualOperationStart = 0;
String currentManualAction = "";

// Ultrasonic sensor variables
unsigned long lastDetectionTime = 0;
bool boatDetectedSensor1 = false;
bool boatDetectedSensor2 = false;
bool boatCurrentlyDetected = false;
bool boatInPassage = false;              // Tracks if a boat is in passage
unsigned long lastExitDetectionTime = 0;
unsigned long lastReverseDetectionTime = 0;
unsigned long lastUltrasonicCheck = 0;
const unsigned long ULTRASONIC_CHECK_INTERVAL = 200;  // Check every 200ms

// Servo objects
Servo servoL;
Servo servoR;

// State durations (updated with emergency timeout for STATE5)
const unsigned long DELAY_STATE1 = 3000;   // 3s for traffic to slow
const unsigned long DELAY_STATE2 = 5000;   // 5s for traffic to clear
const unsigned long DELAY_STATE2B = 3000;  // 3s buffer before opening
const unsigned long DELAY_STATE3 = 10000;  // Max time to open bridge (with limit switch override)
const unsigned long DELAY_STATE4 = 3000;   // 3s yellow warning before green
const unsigned long DELAY_STATE5 = 30000;  // 30s EMERGENCY timeout for boats
const unsigned long DELAY_STATE6 = 3000;   // 3s yellow warning boats
const unsigned long DELAY_STATE7 = 10000;  // Max time to close bridge (with limit switch override)
const unsigned long DELAY_STATE8 = 2000;   // 2s yellow before green

// WebServer
WebServer server(80);

// Authentication
const String adminUsername = "admin";
const String adminPassword = "admin";
String authToken = "";
const String REACT_APP_URL = "http://192.168.4.2:3000";

String generateToken() {
  return "secure_token_" + String(random(100000, 999999));
}

// Function to check if bridge is fully open (limit switch pressed = LOW)
bool isBridgeFullyOpen() {
  return digitalRead(LIMIT_SWITCH_TOP) == LOW;
}

// Function to check if bridge is fully closed (limit switch pressed = LOW)
bool isBridgeFullyClosed() {
  return digitalRead(LIMIT_SWITCH_BOTTOM) == LOW;
}

// FIXED: Function to read distance with error handling
long readUltrasonicDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH, ULTRASONIC_TIMEOUT);
  
  // Validate reading
  if (duration == 0) {
    return -1;  // Sensor timeout or no echo
  }
  
  long distance = duration * 0.0343 / 2;
  
  // Sanity check (HC-SR04 range: 2-400cm)
  if (distance < 2 || distance > 400) {
    return -1;  // Invalid reading
  }
  
  return distance;
}

// FIXED: Function to check if boat is detected (30-50cm range, with CORRECTED size filtering)
bool detectBoat() {
  long distance1 = readUltrasonicDistance(ULTRASONIC_1_TRIG, ULTRASONIC_1_ECHO);
  long distance2 = readUltrasonicDistance(ULTRASONIC_2_TRIG, ULTRASONIC_2_ECHO);
  
  // Check sensor 1 (entry)
  boatDetectedSensor1 = false;
  if (distance1 >= DETECTION_DISTANCE_MIN_CM && distance1 <= DETECTION_DISTANCE_MAX_CM) {
    // FIXED: Small boats are CLOSER (< threshold), not farther
    if (distance1 < SMALL_BOAT_THRESHOLD_CM) {
      Serial.print("🚤 BOAT TOO SMALL - IGNORED | Sensor 1: ");
      Serial.print(distance1);
      Serial.println(" cm (< 35cm threshold)");
    } else {
      boatDetectedSensor1 = true;
    }
  } else if (distance1 == -1) {
    Serial.println("⚠️  Sensor 1 read error");
  }
  
  // Check sensor 2 (exit)
  boatDetectedSensor2 = false;
  if (distance2 >= DETECTION_DISTANCE_MIN_CM && distance2 <= DETECTION_DISTANCE_MAX_CM) {
    if (distance2 < SMALL_BOAT_THRESHOLD_CM) {
      Serial.print("🚤 BOAT TOO SMALL - IGNORED | Sensor 2: ");
      Serial.print(distance2);
      Serial.println(" cm (< 35cm threshold)");
    } else {
      boatDetectedSensor2 = true;
    }
  } else if (distance2 == -1) {
    Serial.println("⚠️  Sensor 2 read error");
  }
  
  // Update real-time detection status
  boatCurrentlyDetected = boatDetectedSensor1 || boatDetectedSensor2;
  
  // Enhanced debug output only if large boat detected
  if (boatCurrentlyDetected) {
    Serial.print("🚤 LARGE BOAT DETECTED! (≥35cm) | Sensor 1: ");
    Serial.print(distance1);
    Serial.print(" cm");
    if (boatDetectedSensor1) Serial.print(" ✓");
    Serial.print(" | Sensor 2: ");
    Serial.print(distance2);
    Serial.print(" cm");
    if (boatDetectedSensor2) Serial.print(" ✓");
    Serial.println();
  }
  
  return boatCurrentlyDetected;
}

void logAllHeaders() {
  Serial.println("Received headers:");
  for (uint8_t i = 0; i < server.headers(); i++) {
    Serial.print("  ");
    Serial.print(server.headerName(i));
    Serial.print(": ");
    Serial.println(server.header(i));
  }
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
  if (!motorActionLogged) {
    Serial.println("🛑 Motor: Stopped");
    motorActionLogged = true;
  }
}

void motorOpen() {
  // Safety check - don't open if already fully open
  if (isBridgeFullyOpen()) {
    Serial.println("⚠️  Motor: Cannot open - already at top limit");
    motorStop();
    return;
  }
  
  digitalWrite(MOTOR_DIRECTION_PIN, HIGH);
  analogWrite(MOTOR_SPEED_PIN, MOTOR_SPEED);
  if (!motorActionLogged) {
    Serial.println("⬆️  Motor: Opening bridge");
    motorActionLogged = true;
  }
}

void motorClose() {
  // Safety check - don't close if already fully closed
  if (isBridgeFullyClosed()) {
    Serial.println("⚠️  Motor: Cannot close - already at bottom limit");
    motorStop();
    return;
  }
  
  digitalWrite(MOTOR_DIRECTION_PIN, LOW);
  analogWrite(MOTOR_SPEED_PIN, MOTOR_SPEED);
  if (!motorActionLogged) {
    Serial.println("⬇️  Motor: Closing bridge");
    motorActionLogged = true;
  }
}

void boomGateRaise() {
  if (!servoL.attached()) servoL.attach(SERVO_L_PIN);
  if (!servoR.attached()) servoR.attach(SERVO_R_PIN);
  delay(50);
  servoL.write(SERVO_RAISED_ANGLE);
  servoR.write(SERVO_RAISED_ANGLE);
  if (!stateActionsLogged) {
    Serial.println("⬆️  Boom Gate: Raised (traffic pass)");
  }
}

void boomGateLower() {
  if (!servoL.attached()) servoL.attach(SERVO_L_PIN);
  if (!servoR.attached()) servoR.attach(SERVO_R_PIN);
  delay(50);
  servoL.write(SERVO_LOWERED_ANGLE);
  servoR.write(SERVO_LOWERED_ANGLE);
  if (!stateActionsLogged) {
    Serial.println("⬇️  Boom Gate: Lowered (stop traffic)");
  }
}

void boomGateHold() {
  servoL.detach();
  servoR.detach();
  if (!stateActionsLogged) {
    Serial.println("🔒 Boom Gate: Detached (holding)");
  }
}

void setLightsForState(State state) {
  resetLights();
  boomGateHold();
  
  switch (state) {
    case STATE0:  // IDLE
      digitalWrite(TRAFFIC_A_GREEN, HIGH);
      digitalWrite(BOAT_B_RED, HIGH);
      boomGateRaise();
      break;
    
    case STATE1:  // BOAT DETECTED
      digitalWrite(TRAFFIC_A_YELLOW, HIGH);
      digitalWrite(BOAT_B_RED, HIGH);
      boomGateRaise();
      break;
    
    case STATE2:  // CLEARING TRAFFIC
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(BOAT_B_RED, HIGH);
      boomGateRaise();  // Still up for vehicles to exit
      break;
    
    case STATE2B:  // TRAFFIC CLEAR CONFIRMATION
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(BOAT_B_RED, HIGH);
      boomGateLower();  // Now lower boom gates
      break;
    
    case STATE3:  // OPENING BRIDGE
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(BOAT_B_RED, HIGH);
      boomGateLower();
      break;
    
    case STATE4:  // BRIDGE FULLY OPEN (YELLOW WARNING)
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(BOAT_B_YELLOW, HIGH);  // Yellow warning for boats
      boomGateLower();
      break;
    
    case STATE5:  // BRIDGE OPEN (WAITING)
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(BOAT_B_GREEN, HIGH);  // Green for boats to pass
      boomGateLower();
      break;
    
    case STATE6:  // STOPPING BOATS
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(BOAT_B_YELLOW, HIGH);
      boomGateLower();
      break;
    
    case STATE7:  // CLOSING BRIDGE
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(BOAT_B_RED, HIGH);
      boomGateLower();
      break;
    
    case STATE8:  // BRIDGE FULLY CLOSED
      digitalWrite(TRAFFIC_A_YELLOW, HIGH);  // Yellow preparing for green
      digitalWrite(BOAT_B_RED, HIGH);
      boomGateRaise();
      break;
  }
  
  if (!stateActionsLogged) {
    Serial.print("💡 Lights set for STATE");
    Serial.print(state);
    Serial.print(": Traffic R=");
    Serial.print(digitalRead(TRAFFIC_A_RED));
    Serial.print(" Y=");
    Serial.print(digitalRead(TRAFFIC_A_YELLOW));
    Serial.print(" G=");
    Serial.print(digitalRead(TRAFFIC_A_GREEN));
    Serial.print("; Boat R=");
    Serial.print(digitalRead(BOAT_B_RED));
    Serial.print(" Y=");
    Serial.print(digitalRead(BOAT_B_YELLOW));
    Serial.print(" G=");
    Serial.println(digitalRead(BOAT_B_GREEN));
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
  Serial.println("👤 Override mode lights set");
}

void addCorsHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type, x-auth-token, X-Auth-Token, X-AUTH-TOKEN");
}

void handleOptions() {
  Serial.println("Handling OPTIONS request");
  addCorsHeaders();
  server.send(204);
}

void handleLogin() {
  Serial.println("🔐 Handling POST /api/login");
  addCorsHeaders();
  
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    Serial.println("Login request body: " + body);
    
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, body);
    
    if (error) {
      Serial.println("❌ JSON parse error: " + String(error.c_str()));
      server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
      return;
    }
    
    String username = doc["username"];
    String password = doc["password"];
    
    if (username == adminUsername && password == adminPassword) {
      // Invalidate previous token
      if (authToken != "") {
        Serial.println("⚠️  New login - invalidating previous token");
      }
      authToken = generateToken();
      Serial.println("✅ Login successful, token: " + authToken);
      
      DynamicJsonDocument resp(256);
      resp["success"] = true;
      resp["token"] = authToken;
      String json;
      serializeJson(resp, json);
      server.send(200, "application/json", json);
    } else {
      Serial.println("❌ Login failed: Invalid credentials");
      DynamicJsonDocument resp(256);
      resp["success"] = false;
      resp["message"] = "Invalid credentials";
      String json;
      serializeJson(resp, json);
      server.send(401, "application/json", json);
    }
  } else {
    Serial.println("❌ Login failed: No body");
    server.send(400, "application/json", "{\"error\":\"No body\"}");
  }
}

void handleState() {
  addCorsHeaders();
  
  String token = server.header("x-auth-token");
  if (token == "") token = server.header("X-Auth-Token");
  if (token == "") token = server.header("X-AUTH-TOKEN");
  if (token == "") token = server.arg("token");
  
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
  Serial.println("========================================");
  Serial.println("📨 Handling POST /api/command");
  addCorsHeaders();
  
  String token = server.header("x-auth-token");
  if (token == "") token = server.header("X-Auth-Token");
  if (token == "") token = server.header("X-AUTH-TOKEN");
  if (token == "") token = server.arg("token");
  
  if (token != authToken || authToken == "") {
    Serial.println("❌ Command request failed: Unauthorized");
    server.send(401, "application/json", "{\"error\":\"Unauthorized\"}");
    return;
  }
  
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    Serial.println("Command body: " + body);
    
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, body);
    
    if (error) {
      Serial.println("❌ JSON parse error: " + String(error.c_str()));
      server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
      return;
    }
    
    String action = doc["action"];
    Serial.println("Action: " + action + ", Override: " + String(manualOverrideActive) + ", State: " + String(currentState));
    
    if (action == "enableOverride") {
      Serial.println(">>> 👤 ENABLING MANUAL OVERRIDE <<<");
      manualOverrideActive = true;
      motorStop();
      setLightsForOverride();
      server.send(200, "application/json", "{\"success\":true}");
      return;
    }
    
    if (action == "disableOverride") {
      Serial.println(">>> 🤖 DISABLING MANUAL OVERRIDE <<<");
      manualOverrideActive = false;
      manualOperationInProgress = false;  // Stop any manual operations
      if (bridgeOpen) {
        currentState = STATE5;
      } else {
        currentState = STATE0;
      }
      stateActionsLogged = false;
      motorActionLogged = false;
      setLightsForState(currentState);
      stateStartTime = millis();
      server.send(200, "application/json", "{\"success\":true}");
      return;
    }
    
    if (action == "trafficRed") {
      if (manualOverrideActive) {
        Serial.println(">>> 🔴 OVERRIDE: Traffic RED <<<");
        digitalWrite(TRAFFIC_A_RED, HIGH);
        digitalWrite(TRAFFIC_A_YELLOW, LOW);
        digitalWrite(TRAFFIC_A_GREEN, LOW);
        server.send(200, "application/json", "{\"success\":true}");
      } else {
        server.send(400, "application/json", "{\"error\":\"Not in override mode\"}");
      }
      return;
    }
    
    if (action == "trafficYellow") {
      if (manualOverrideActive) {
        Serial.println(">>> 🟡 OVERRIDE: Traffic YELLOW <<<");
        digitalWrite(TRAFFIC_A_RED, LOW);
        digitalWrite(TRAFFIC_A_YELLOW, HIGH);
        digitalWrite(TRAFFIC_A_GREEN, LOW);
        server.send(200, "application/json", "{\"success\":true}");
      } else {
        server.send(400, "application/json", "{\"error\":\"Not in override mode\"}");
      }
      return;
    }
    
    if (action == "trafficGreen") {
      if (manualOverrideActive) {
        Serial.println(">>> 🟢 OVERRIDE: Traffic GREEN <<<");
        digitalWrite(TRAFFIC_A_RED, LOW);
        digitalWrite(TRAFFIC_A_YELLOW, LOW);
        digitalWrite(TRAFFIC_A_GREEN, HIGH);
        server.send(200, "application/json", "{\"success\":true}");
      } else {
        server.send(400, "application/json", "{\"error\":\"Not in override mode\"}");
      }
      return;
    }
    
    if (action == "boatRed") {
      if (manualOverrideActive) {
        Serial.println(">>> 🔴 OVERRIDE: Boat RED <<<");
        digitalWrite(BOAT_B_RED, HIGH);
        digitalWrite(BOAT_B_YELLOW, LOW);
        digitalWrite(BOAT_B_GREEN, LOW);
        server.send(200, "application/json", "{\"success\":true}");
      } else {
        server.send(400, "application/json", "{\"error\":\"Not in override mode\"}");
      }
      return;
    }
    
    if (action == "boatYellow") {
      if (manualOverrideActive) {
        Serial.println(">>> 🟡 OVERRIDE: Boat YELLOW <<<");
        digitalWrite(BOAT_B_RED, LOW);
        digitalWrite(BOAT_B_YELLOW, HIGH);
        digitalWrite(BOAT_B_GREEN, LOW);
        server.send(200, "application/json", "{\"success\":true}");
      } else {
        server.send(400, "application/json", "{\"error\":\"Not in override mode\"}");
      }
      return;
    }
    
    if (action == "boatGreen") {
      if (manualOverrideActive) {
        Serial.println(">>> 🟢 OVERRIDE: Boat GREEN <<<");
        digitalWrite(BOAT_B_RED, LOW);
        digitalWrite(BOAT_B_YELLOW, LOW);
        digitalWrite(BOAT_B_GREEN, HIGH);
        server.send(200, "application/json", "{\"success\":true}");
      } else {
        server.send(400, "application/json", "{\"error\":\"Not in override mode\"}");
      }
      return;
    }
    
    if (action == "open") {
      if (manualOverrideActive) {
        Serial.println(">>> ⬆️  OVERRIDE: Starting bridge opening <<<");
        bridgeOpen = true;
        motorActionLogged = false;
        setLightsForOverride();
        motorOpen();
        digitalWrite(LED_PIN, HIGH);
        manualOperationInProgress = true;
        manualOperationStart = millis();
        currentManualAction = "opening";
        server.send(200, "application/json", "{\"success\":true,\"status\":\"opening\"}");
      } else {
        if (currentState == STATE0) {
          Serial.println(">>> 🤖 AUTO: Starting open sequence <<<");
          currentState = STATE1;
          boatInPassage = true;
          stateActionsLogged = false;
          motorActionLogged = false;
          setLightsForState(currentState);
          stateStartTime = millis();
          digitalWrite(LED_PIN, HIGH);
        }
        server.send(200, "application/json", "{\"success\":true}");
      }
      return;
    }
    
    if (action == "close") {
      if (manualOverrideActive) {
        Serial.println(">>> ⬇️  OVERRIDE: Starting bridge closing <<<");
        bridgeOpen = false;
        motorActionLogged = false;
        setLightsForOverride();
        motorClose();
        digitalWrite(LED_PIN, LOW);
        manualOperationInProgress = true;
        manualOperationStart = millis();
        currentManualAction = "closing";
        server.send(200, "application/json", "{\"success\":true,\"status\":\"closing\"}");
      } else {
        if (bridgeOpen && currentState == STATE5) {
          Serial.println(">>> 🤖 AUTO: Starting close sequence <<<");
          currentState = STATE6;
          boatInPassage = false;
          stateActionsLogged = false;
          motorActionLogged = false;
          setLightsForState(currentState);
          stateStartTime = millis();
        }
        server.send(200, "application/json", "{\"success\":true}");
      }
      return;
    }
    
    if (action == "clear") {
      Serial.println(">>> 🔄 CLEAR: Reset to STATE0 <<<");
      manualOverrideActive = false;
      manualOperationInProgress = false;
      currentState = STATE0;
      boatInPassage = false;
      stateActionsLogged = false;
      motorActionLogged = false;
      setLightsForState(currentState);
      stateStartTime = millis();
      bridgeOpen = false;
      motorStop();
      digitalWrite(LED_PIN, LOW);
      server.send(200, "application/json", "{\"success\":true}");
      return;
    }
    
    server.send(400, "application/json", "{\"error\":\"Unknown action\"}");
  } else {
    server.send(400, "application/json", "{\"error\":\"No body\"}");
  }
  Serial.println("========================================");
}

void handleLogout() {
  Serial.println("🚪 Handling GET /api/logout");
  addCorsHeaders();
  
  String token = server.header("x-auth-token");
  if (token == "") token = server.header("X-Auth-Token");
  if (token == "") token = server.header("X-AUTH-TOKEN");
  if (token == "") token = server.arg("token");
  
  if (token != authToken || authToken == "") {
    server.send(401, "application/json", "{\"error\":\"Unauthorized\"}");
    return;
  }
  
  authToken = "";
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
  
  // Setup ultrasonic sensor pins
  pinMode(ULTRASONIC_1_TRIG, OUTPUT);
  pinMode(ULTRASONIC_1_ECHO, INPUT);
  pinMode(ULTRASONIC_2_TRIG, OUTPUT);
  pinMode(ULTRASONIC_2_ECHO, INPUT);
  
  // Setup limit switch pins (INPUT_PULLUP for active LOW)
  pinMode(LIMIT_SWITCH_TOP, INPUT_PULLUP);
  pinMode(LIMIT_SWITCH_BOTTOM, INPUT_PULLUP);
  
  setLightsForState(currentState);
  motorStop();
  stateStartTime = millis();
  bridgeOpen = false;
  
  servoL.attach(SERVO_L_PIN);
  servoR.attach(SERVO_R_PIN);
  boomGateRaise();
  
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
  
  Serial.begin(115200);
  Serial.println("\n\n========================================");
  Serial.println("🌉 ESP32 Bridge Control System v2.1");
  Serial.println("========================================");
  Serial.println("States: 0=IDLE, 1=BOAT_DETECTED, 2=CLEARING, 2B=CONFIRMED, 3=OPENING,");
  Serial.println("        4=OPEN_YELLOW, 5=OPEN_WAITING, 6=STOPPING, 7=CLOSING, 8=CLOSED_YELLOW");
  
  Serial.println("\n📡 Ultrasonic Sensors:");
  Serial.println("  Sensor 1 (Entry): Trig=D15, Echo=D2");
  Serial.println("  Sensor 2 (Exit): Trig=D4, Echo=D16");
  Serial.print("  Detection range: ");
  Serial.print(DETECTION_DISTANCE_MIN_CM);
  Serial.print("-");
  Serial.print(DETECTION_DISTANCE_MAX_CM);
  Serial.println(" cm");
  Serial.print("  Small boat threshold: <");
  Serial.print(SMALL_BOAT_THRESHOLD_CM);
  Serial.println(" cm IGNORED (boats closer than threshold)");
  Serial.print("  Large boat threshold: ≥");
  Serial.print(SMALL_BOAT_THRESHOLD_CM);
  Serial.println(" cm DETECTED (boats farther than threshold)");
  Serial.print("  Min detection time: ");
  Serial.print(MIN_DETECTION_TIME);
  Serial.println(" ms");
  
  Serial.println("\n🔧 Limit Switches:");
  Serial.println("  Top (fully open): D25");
  Serial.println("  Bottom (fully closed): D33");
  Serial.print("  Initial state - Top: ");
  Serial.print(isBridgeFullyOpen() ? "PRESSED" : "RELEASED");
  Serial.print(", Bottom: ");
  Serial.println(isBridgeFullyClosed() ? "PRESSED" : "RELEASED");
  
  Serial.println("\n📶 Starting WiFi Access Point...");
  WiFi.softAP(ssid, ap_password);
  delay(500);
  Serial.print("  SSID: ");
  Serial.println(ssid);
  Serial.print("  Password: ");
  Serial.println(ap_password);
  Serial.print("  AP IP Address: ");
  Serial.println(WiFi.softAPIP());
  
  Serial.println("\n🌐 Registering HTTP endpoints...");
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
  Serial.println("✅ HTTP server started on port 80");
  Serial.println("========================================");
  Serial.println("🚀 System ready! Monitoring for boats (30-50cm, ≥35cm for large)...\n");
}

void loop() {
  server.handleClient();
  
  unsigned long currentTime = millis();
  
  // Handle manual operations non-blocking
  if (manualOverrideActive && manualOperationInProgress) {
    if (currentManualAction == "opening") {
      if (isBridgeFullyOpen()) {
        Serial.println("✅ OVERRIDE: Top limit reached");
        motorStop();
        manualOperationInProgress = false;
      } else if (currentTime - manualOperationStart >= DELAY_STATE3) {
        Serial.println("⏰ OVERRIDE: Open timeout");
        motorStop();
        manualOperationInProgress = false;
      }
    } else if (currentManualAction == "closing") {
      if (isBridgeFullyClosed()) {
        Serial.println("✅ OVERRIDE: Bottom limit reached");
        motorStop();
        manualOperationInProgress = false;
      } else if (currentTime - manualOperationStart >= DELAY_STATE7) {
        Serial.println("⏰ OVERRIDE: Close timeout");
        motorStop();
        manualOperationInProgress = false;
      }
    }
  }
  
  // Always update boat detection status
  if (currentTime - lastUltrasonicCheck >= ULTRASONIC_CHECK_INTERVAL) {
    lastUltrasonicCheck = currentTime;
    detectBoat();  // Updates boatDetectedSensor1/2, boatCurrentlyDetected
    
    // Only trigger state transition in STATE0 and automatic mode, using entry sensor
    if (!manualOverrideActive && currentState == STATE0 && boatDetectedSensor1) {
      if (lastDetectionTime == 0) {
        lastDetectionTime = currentTime;
        Serial.println("⚠️  Entry detection started - waiting for continuous detection (500ms)...");
      } else if (currentTime - lastDetectionTime >= MIN_DETECTION_TIME) {
        Serial.println("✅ Continuous large boat detection confirmed on entry → STATE1");
        currentState = STATE1;
        boatInPassage = true;
        stateActionsLogged = false;
        motorActionLogged = false;
        setLightsForState(currentState);
        stateStartTime = millis();
        digitalWrite(LED_PIN, HIGH);
        lastDetectionTime = 0;
      }
    } else if (!boatDetectedSensor1) {
      if (lastDetectionTime != 0) {
        Serial.println("✗ Entry detection lost - resetting timer");
      }
      lastDetectionTime = 0;
    }
  }
  
  // Skip automatic transitions in override mode
  if (manualOverrideActive) {
    return;
  }
  
  // AUTOMATIC MODE STATE MACHINE
  switch (currentState) {
    case STATE0:  // IDLE
      motorStop();
      break;
    
    case STATE1:  // BOAT DETECTED
      motorStop();
      if (currentTime - stateStartTime >= DELAY_STATE1) {
        Serial.println("STATE1 → STATE2 (clearing traffic)");
        currentState = STATE2;
        stateActionsLogged = false;
        motorActionLogged = false;
        setLightsForState(currentState);
        stateStartTime = currentTime;
      }
      break;
    
    case STATE2:  // CLEARING TRAFFIC
      motorStop();
      if (currentTime - stateStartTime >= DELAY_STATE2) {
        Serial.println("STATE2 → STATE2B (traffic cleared, lowering boom gates)");
        currentState = STATE2B;
        stateActionsLogged = false;
        motorActionLogged = false;
        setLightsForState(currentState);
        stateStartTime = currentTime;
      }
      break;
    
    case STATE2B:  // TRAFFIC CLEAR CONFIRMATION
      motorStop();
      if (currentTime - stateStartTime >= DELAY_STATE2B) {
        Serial.println("STATE2B → STATE3 (opening bridge)");
        currentState = STATE3;
        stateActionsLogged = false;
        motorActionLogged = false;
        setLightsForState(currentState);
        stateStartTime = currentTime;
      }
      break;
    
    case STATE3:  // OPENING BRIDGE
      motorOpen();
      
      if (isBridgeFullyOpen()) {
        Serial.println("STATE3 → STATE4 (top limit switch reached)");
        currentState = STATE4;
        stateActionsLogged = false;
        motorActionLogged = false;
        setLightsForState(currentState);
        motorStop();
        stateStartTime = currentTime;
        bridgeOpen = true;
      } else if (currentTime - stateStartTime >= DELAY_STATE3) {
        Serial.println("STATE3 → STATE4 (timeout - WARNING: limit switch not triggered)");
        currentState = STATE4;
        stateActionsLogged = false;
        motorActionLogged = false;
        setLightsForState(currentState);
        motorStop();
        stateStartTime = currentTime;
        bridgeOpen = true;
      }
      break;
    
    case STATE4:  // BRIDGE FULLY OPEN (YELLOW WARNING)
      motorStop();
      if (currentTime - stateStartTime >= DELAY_STATE4) {
        Serial.println("STATE4 → STATE5 (boats can pass - GREEN)");
        currentState = STATE5;
        stateActionsLogged = false;
        motorActionLogged = false;
        setLightsForState(currentState);
        stateStartTime = currentTime;
      }
      break;
    
    case STATE5:  // BRIDGE OPEN (WAITING)
      motorStop();
      
      // Check for exit confirmation
      if (boatDetectedSensor2) {
        if (lastExitDetectionTime == 0) {
          lastExitDetectionTime = currentTime;
          Serial.println("🚤 Boat detected at exit sensor...");
        } else if (currentTime - lastExitDetectionTime >= MIN_DETECTION_TIME) {
          Serial.println("✅ Boat exit confirmed → STATE6");
          boatInPassage = false;
          currentState = STATE6;
          stateActionsLogged = false;
          motorActionLogged = false;
          setLightsForState(currentState);
          stateStartTime = currentTime;
          lastExitDetectionTime = 0;
        }
      } else {
        lastExitDetectionTime = 0;
      }
      
      // Check for possible reverse on entry
      if (boatDetectedSensor1) {
        if (lastReverseDetectionTime == 0) {
          lastReverseDetectionTime = currentTime;
          Serial.println("⚠️  Boat detected moving back to entry sensor...");
        } else if (currentTime - lastReverseDetectionTime >= MIN_DETECTION_TIME) {
          Serial.println("⚠️  Boat reverse confirmed → STATE6");
          boatInPassage = false;
          currentState = STATE6;
          stateActionsLogged = false;
          motorActionLogged = false;
          setLightsForState(currentState);
          stateStartTime = currentTime;
          lastReverseDetectionTime = 0;
        }
      } else {
        lastReverseDetectionTime = 0;
      }
      
      // ADDED: Emergency timeout if boat doesn't exit OR reverse
      if (currentTime - stateStartTime >= DELAY_STATE5) {
        Serial.println("⏰ EMERGENCY TIMEOUT: Boat didn't exit in 30s → STATE6");
        Serial.println("   (Possible stopped boat or sensor malfunction)");
        boatInPassage = false;
        currentState = STATE6;
        stateActionsLogged = false;
        motorActionLogged = false;
        setLightsForState(currentState);
        stateStartTime = currentTime;
        lastExitDetectionTime = 0;
        lastReverseDetectionTime = 0;
      }
      break;
    
    case STATE6:  // STOPPING BOATS
      motorStop();
      if (currentTime - stateStartTime >= DELAY_STATE6) {
        Serial.println("STATE6 → STATE7 (closing bridge)");
        currentState = STATE7;
        stateActionsLogged = false;
        motorActionLogged = false;
        setLightsForState(currentState);
        stateStartTime = currentTime;
      }
      break;
    
    case STATE7:  // CLOSING BRIDGE
      motorClose();
      
      if (isBridgeFullyClosed()) {
        Serial.println("STATE7 → STATE8 (bottom limit switch reached)");
        currentState = STATE8;
        stateActionsLogged = false;
        motorActionLogged = false;
        setLightsForState(currentState);
        motorStop();
        stateStartTime = currentTime;
        bridgeOpen = false;
      } else if (currentTime - stateStartTime >= DELAY_STATE7) {
        Serial.println("STATE7 → STATE8 (timeout - WARNING: limit switch not triggered)");
        currentState = STATE8;
        stateActionsLogged = false;
        motorActionLogged = false;
        setLightsForState(currentState);
        motorStop();
        stateStartTime = currentTime;
        bridgeOpen = false;
      }
      break;
    
    case STATE8:  // BRIDGE FULLY CLOSED
      motorStop();
      if (currentTime - stateStartTime >= DELAY_STATE8) {
        Serial.println("STATE8 → STATE0 (returning to idle)");
        currentState = STATE0;
        stateActionsLogged = false;
        motorActionLogged = false;
        setLightsForState(currentState);
        stateStartTime = currentTime;
        digitalWrite(LED_PIN, LOW);
      }
      break;
  }
}