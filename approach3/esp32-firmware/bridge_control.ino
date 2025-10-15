#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

// WiFi AP credentials
const char* ssid = "ESP32_Bridge";
const char* ap_password = "12345678";

// Pin definitions
const int BUTTON_PIN = 2;
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

// Servo angles
const int SERVO_RAISED_ANGLE = 0;
const int SERVO_LOWERED_ANGLE = 90;

// Motor speed
const int MOTOR_SPEED = 255;

// Enum for states
enum State {
  STATE0, STATE1, STATE2, STATE3, STATE4,
  STATE5, STATE6, STATE7, STATE8, STATE9
};

// State variables
State currentState = STATE0;
unsigned long stateStartTime = 0;
bool bridgeOpen = false;
bool stateActionsLogged = false;
bool motorActionLogged = false;

// *** NEW: Manual override flag ***
bool manualOverrideActive = false;

// Servo objects
Servo servoL;
Servo servoR;

// State durations
const unsigned long DELAY_STATE1 = 2000;
const unsigned long DELAY_STATE2 = 2000;
const unsigned long DELAY_STATE3 = 2000;
const unsigned long DELAY_STATE4 = 5000;
const unsigned long DELAY_STATE5 = 5000;
const unsigned long DELAY_STATE6 = 2000;
const unsigned long DELAY_STATE7 = 2000;
const unsigned long DELAY_STATE8 = 5000;
const unsigned long DELAY_STATE9 = 2000;

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
    Serial.println("Motor: Stopped");
    motorActionLogged = true;
  }
}

void motorOpen() {
  digitalWrite(MOTOR_DIRECTION_PIN, HIGH);
  analogWrite(MOTOR_SPEED_PIN, MOTOR_SPEED);
  if (!motorActionLogged) {
    Serial.println("Motor: Opening bridge");
    motorActionLogged = true;
  }
}

void motorClose() {
  digitalWrite(MOTOR_DIRECTION_PIN, LOW);
  analogWrite(MOTOR_SPEED_PIN, MOTOR_SPEED);
  if (!motorActionLogged) {
    Serial.println("Motor: Closing bridge");
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
    Serial.println("Boom Gate: Raised (traffic pass)");
  }
}

void boomGateLower() {
  if (!servoL.attached()) servoL.attach(SERVO_L_PIN);
  if (!servoR.attached()) servoR.attach(SERVO_R_PIN);
  delay(50);
  servoL.write(SERVO_LOWERED_ANGLE);
  servoR.write(SERVO_LOWERED_ANGLE);
  if (!stateActionsLogged) {
    Serial.println("Boom Gate: Lowered (stop traffic)");
  }
}

void boomGateHold() {
  servoL.detach();
  servoR.detach();
  if (!stateActionsLogged) {
    Serial.println("Boom Gate: Detached (holding, no jitter)");
  }
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
    case STATE3:
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(BOAT_B_RED, HIGH);
      boomGateRaise();
      break;
    case STATE4:
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(BOAT_B_YELLOW, HIGH);
      boomGateLower();
      break;
    case STATE5:
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(BOAT_B_GREEN, HIGH);
      break;
    case STATE6:
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(BOAT_B_YELLOW, HIGH);
      break;
    case STATE7:
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(BOAT_B_RED, HIGH);
      break;
    case STATE8:
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(BOAT_B_RED, HIGH);
      boomGateRaise();
      break;
    case STATE9:
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(BOAT_B_RED, HIGH);
      boomGateRaise();
      break;
  }
  if (!stateActionsLogged) {
    Serial.print("Lights set: Traffic Red=");
    Serial.print(digitalRead(TRAFFIC_A_RED));
    Serial.print(", Yellow=");
    Serial.print(digitalRead(TRAFFIC_A_YELLOW));
    Serial.print(", Green=");
    Serial.print(digitalRead(TRAFFIC_A_GREEN));
    Serial.print("; Boat Red=");
    Serial.print(digitalRead(BOAT_B_RED));
    Serial.print(", Yellow=");
    Serial.print(digitalRead(BOAT_B_YELLOW));
    Serial.print(", Green=");
    Serial.println(digitalRead(BOAT_B_GREEN));
    stateActionsLogged = true;
  }
}

// *** NEW: Set lights for manual override mode ***
void setLightsForOverride() {
  resetLights();
  if (bridgeOpen) {
    // Bridge open in override mode
    digitalWrite(TRAFFIC_A_RED, HIGH);
    digitalWrite(BOAT_B_YELLOW, HIGH);
    boomGateLower();
  } else {
    // Bridge closed in override mode
    digitalWrite(TRAFFIC_A_RED, HIGH);
    digitalWrite(BOAT_B_RED, HIGH);
    boomGateRaise();
  }
  Serial.println("Override mode lights set");
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
  Serial.println("Handling POST /api/login");
  addCorsHeaders();
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    Serial.println("Login request body: " + body);
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, body);
    if (error) {
      Serial.println("JSON parse error: " + String(error.c_str()));
      server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
      return;
    }
    String username = doc["username"];
    String password = doc["password"];
    if (username == adminUsername && password == adminPassword) {
      Serial.println("Current authToken before login: '" + authToken + "'");
      authToken = generateToken();
      Serial.println("Login successful, token: " + authToken);
      DynamicJsonDocument resp(256);
      resp["success"] = true;
      resp["token"] = authToken;
      String json;
      serializeJson(resp, json);
      server.send(200, "application/json", json);
    } else {
      Serial.println("Login failed: Invalid credentials");
      DynamicJsonDocument resp(256);
      resp["success"] = false;
      resp["message"] = "Invalid credentials";
      String json;
      serializeJson(resp, json);
      server.send(401, "application/json", json);
    }
  } else {
    Serial.println("Login failed: No body");
    server.send(400, "application/json", "{\"error\":\"No body\"}");
  }
}

void handleState() {
  Serial.println("Handling GET /api/state");
  addCorsHeaders();
  logAllHeaders();
  String token = server.header("x-auth-token");
  if (token == "") token = server.header("X-Auth-Token");
  if (token == "") token = server.header("X-AUTH-TOKEN");
  if (token == "") token = server.arg("token");
  
  Serial.print("State request token: '");
  Serial.print(token);
  Serial.println("'");
  
  if (token != authToken || authToken == "") {
    Serial.println("State request failed: Unauthorized");
    server.send(401, "application/json", "{\"error\":\"Unauthorized\"}");
    return;
  }
  
  DynamicJsonDocument doc(1024);
  doc["currentState"] = currentState;
  doc["bridgeState"] = bridgeOpen;
  doc["manualOverride"] = manualOverrideActive;  // *** NEW: Send override status ***
  doc["redLedA"] = digitalRead(TRAFFIC_A_RED);
  doc["yellowLedA"] = digitalRead(TRAFFIC_A_YELLOW);
  doc["greenLedA"] = digitalRead(TRAFFIC_A_GREEN);
  doc["redLedB"] = digitalRead(BOAT_B_RED);
  doc["yellowLedB"] = digitalRead(BOAT_B_YELLOW);
  doc["greenLedB"] = digitalRead(BOAT_B_GREEN);
  
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void handleCommand() {
  Serial.println("Handling POST /api/command");
  addCorsHeaders();
  logAllHeaders();
  
  String token = server.header("x-auth-token");
  if (token == "") token = server.header("X-Auth-Token");
  if (token == "") token = server.header("X-AUTH-TOKEN");
  if (token == "") token = server.arg("token");
  
  if (token != authToken || authToken == "") {
    Serial.println("Command request failed: Unauthorized");
    server.send(401, "application/json", "{\"error\":\"Unauthorized\"}");
    return;
  }
  
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    Serial.println("Command request body: " + body);
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, body);
    if (error) {
      Serial.println("JSON parse error: " + String(error.c_str()));
      server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
      return;
    }
    
    String action = doc["action"];
    
    // *** NEW: Handle override mode commands ***
    if (action == "enableOverride") {
      Serial.println("Command: Enable Manual Override");
      manualOverrideActive = true;
      motorStop();
      setLightsForOverride();
      server.send(200, "application/json", "{\"success\":true}");
      return;
    }
    
    if (action == "disableOverride") {
      Serial.println("Command: Disable Manual Override - Returning to Auto Mode");
      manualOverrideActive = false;
      // Return to appropriate state based on bridge position
      if (bridgeOpen) {
        currentState = STATE5;  // Resume at boat passing
      } else {
        currentState = STATE0;  // Resume at default
      }
      stateActionsLogged = false;
      motorActionLogged = false;
      setLightsForState(currentState);
      stateStartTime = millis();
      server.send(200, "application/json", "{\"success\":true}");
      return;
    }
    
    // *** MODIFIED: Handle open/close based on mode ***
    if (action == "open") {
      if (manualOverrideActive) {
        // Manual override mode: Direct control
        Serial.println("Override Command: Opening bridge manually");
        bridgeOpen = true;
        motorActionLogged = false;
        setLightsForOverride();
        motorOpen();
        // Motor will be stopped by a timer or separate stop command
        // For safety, we'll stop after expected duration
        delay(DELAY_STATE4);  // Wait for bridge to open
        motorStop();
        digitalWrite(LED_PIN, HIGH);
      } else {
        // Automatic mode: Start sequence
        if (currentState == STATE0) {
          Serial.println("Auto Command: Open bridge - Starting sequence");
          currentState = STATE1;
          stateActionsLogged = false;
          motorActionLogged = false;
          setLightsForState(currentState);
          stateStartTime = millis();
          digitalWrite(LED_PIN, HIGH);
        }
      }
      server.send(200, "application/json", "{\"success\":true}");
      return;
    }
    
    if (action == "close") {
      if (manualOverrideActive) {
        // Manual override mode: Direct control
        Serial.println("Override Command: Closing bridge manually");
        bridgeOpen = false;
        motorActionLogged = false;
        setLightsForOverride();
        motorClose();
        // Motor will be stopped after expected duration
        delay(DELAY_STATE8);  // Wait for bridge to close
        motorStop();
        digitalWrite(LED_PIN, LOW);
      } else {
        // Automatic mode: Can close from STATE5
        if (bridgeOpen && currentState == STATE5) {
          Serial.println("Auto Command: Close bridge - Starting close sequence");
          currentState = STATE6;
          stateActionsLogged = false;
          motorActionLogged = false;
          setLightsForState(currentState);
          stateStartTime = millis();
          digitalWrite(LED_PIN, LOW);
        }
      }
      server.send(200, "application/json", "{\"success\":true}");
      return;
    }
    
    if (action == "clear") {
      Serial.println("Command: Clear - Resetting to STATE0");
      manualOverrideActive = false;  // Also disable override
      currentState = STATE0;
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
    Serial.println("Command failed: No body");
    server.send(400, "application/json", "{\"error\":\"No body\"}");
  }
}

void handleLogout() {
  Serial.println("Handling GET /api/logout");
  addCorsHeaders();
  logAllHeaders();
  
  String token = server.header("x-auth-token");
  if (token == "") token = server.header("X-Auth-Token");
  if (token == "") token = server.header("X-AUTH-TOKEN");
  if (token == "") token = server.arg("token");
  
  if (token != authToken || authToken == "") {
    Serial.println("Logout request failed: Unauthorized");
    server.send(401, "application/json", "{\"error\":\"Unauthorized\"}");
    return;
  }
  
  authToken = "";
  server.send(200, "application/json", "{\"success\":true}");
}

void handleRoot() {
  Serial.println("Handling GET /");
  addCorsHeaders();
  server.sendHeader("Location", REACT_APP_URL);
  server.send(302, "text/plain", "Redirecting to React app...");
}

void setup() {
  randomSeed(analogRead(34));
  
  pinMode(TRAFFIC_A_RED, OUTPUT);
  pinMode(TRAFFIC_A_YELLOW, OUTPUT);
  pinMode(TRAFFIC_A_GREEN, OUTPUT);
  pinMode(BOAT_B_RED, OUTPUT);
  pinMode(BOAT_B_YELLOW, OUTPUT);
  pinMode(BOAT_B_GREEN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(MOTOR_DIRECTION_PIN, OUTPUT);
  pinMode(MOTOR_SPEED_PIN, OUTPUT);
  
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
  Serial.println("Starting ESP32 Bridge Control with Manual Override...");
  
  WiFi.softAP(ssid, ap_password);
  delay(500);
  Serial.print("AP IP address: ");
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
  Serial.println("HTTP server started with override capability");
}

unsigned long lastButtonPress = 0;

void loop() {
  server.handleClient();
  
  // *** MODIFIED: Only allow button in automatic mode ***
  if (!manualOverrideActive && digitalRead(BUTTON_PIN) == LOW && millis() - lastButtonPress > 200) {
    lastButtonPress = millis();
    if (currentState == STATE0) {
      Serial.println("Button pressed: Ship detected, transitioning to STATE1");
      currentState = STATE1;
      stateActionsLogged = false;
      motorActionLogged = false;
      setLightsForState(currentState);
      stateStartTime = millis();
      digitalWrite(LED_PIN, HIGH);
    }
  }
  
  // *** MODIFIED: Skip automatic transitions if in override mode ***
  if (manualOverrideActive) {
    return;  // Skip all automatic state transitions
  }
  
  // *** AUTOMATIC MODE STATE MACHINE ***
  unsigned long currentTime = millis();
  switch (currentState) {
    case STATE0:
      motorStop();
      break;
      
    case STATE1:
      motorStop();
      if (currentTime - stateStartTime >= DELAY_STATE1) {
        Serial.println("Transitioning to STATE2");
        currentState = STATE2;
        stateActionsLogged = false;
        motorActionLogged = false;
        setLightsForState(currentState);
        stateStartTime = currentTime;
      }
      break;
      
    case STATE2:
      motorStop();
      if (currentTime - stateStartTime >= DELAY_STATE2) {
        Serial.println("Transitioning to STATE3");
        currentState = STATE3;
        stateActionsLogged = false;
        motorActionLogged = false;
        setLightsForState(currentState);
        stateStartTime = currentTime;
        bridgeOpen = true;
      }
      break;
      
    case STATE3:
      motorStop();
      if (currentTime - stateStartTime >= DELAY_STATE3) {
        Serial.println("Transitioning to STATE4, bridge opening");
        currentState = STATE4;
        stateActionsLogged = false;
        motorActionLogged = false;
        setLightsForState(currentState);
        stateStartTime = currentTime;
        bridgeOpen = true;
      }
      break;
      
    case STATE4:
      motorOpen();
      if (currentTime - stateStartTime >= DELAY_STATE4) {
        Serial.println("Transitioning to STATE5");
        currentState = STATE5;
        stateActionsLogged = false;
        motorActionLogged = false;
        setLightsForState(currentState);
        motorStop();
        stateStartTime = currentTime;
      }
      break;
      
    case STATE5:
      motorStop();
      if (currentTime - stateStartTime >= DELAY_STATE5) {
        Serial.println("Transitioning to STATE6");
        currentState = STATE6;
        stateActionsLogged = false;
        motorActionLogged = false;
        setLightsForState(currentState);
        stateStartTime = currentTime;
      }
      break;
      
    case STATE6:
      motorStop();
      if (currentTime - stateStartTime >= DELAY_STATE6) {
        Serial.println("Transitioning to STATE7");
        currentState = STATE7;
        stateActionsLogged = false;
        motorActionLogged = false;
        setLightsForState(currentState);
        stateStartTime = currentTime;
      }
      break;
      
    case STATE7:
      motorStop();
      if (currentTime - stateStartTime >= DELAY_STATE7) {
        Serial.println("Transitioning to STATE8");
        currentState = STATE8;
        stateActionsLogged = false;
        motorActionLogged = false;
        setLightsForState(currentState);
        stateStartTime = currentTime;
        bridgeOpen = false;
      }
      break;
      
    case STATE8:
      motorClose();
      if (currentTime - stateStartTime >= DELAY_STATE8) {
        Serial.println("Transitioning to STATE9, bridge closing");
        currentState = STATE9;
        stateActionsLogged = false;
        motorActionLogged = false;
        setLightsForState(currentState);
        motorStop();
        stateStartTime = currentTime;
        digitalWrite(LED_PIN, LOW);
      }
      break;
      
    case STATE9:
      motorStop();
      if (currentTime - stateStartTime >= DELAY_STATE9) {
        Serial.println("Transitioning to STATE0");
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