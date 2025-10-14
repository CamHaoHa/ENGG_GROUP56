
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>  // ESP32-specific Servo library (uses 'Servo' class)

// WiFi AP credentials
const char* ssid = "ESP32_Bridge";
const char* ap_password = "12345678";

// Pin definitions
const int BUTTON_PIN = 2;  // GPIO2 (input, onboard LED compatible)
const int LED_PIN = 26;    // Free output pin
const int TRAFFIC_A_RED = 32;
const int TRAFFIC_A_YELLOW = 14;  // GPIO14 (output-capable)
const int TRAFFIC_A_GREEN = 27;   // GPIO27 (output-capable)
const int BOAT_B_RED = 23;
const int BOAT_B_YELLOW = 22;
const int BOAT_B_GREEN = 21;
const int MOTOR_DIRECTION_PIN = 13;
const int MOTOR_SPEED_PIN = 12;
const int SERVO_L_PIN = 18;
const int SERVO_R_PIN = 19;

// Servo angles for boom gate
const int SERVO_RAISED_ANGLE = 0;    // Boom gate up (open for traffic)
const int SERVO_LOWERED_ANGLE = 90;  // Boom gate down (closed, stops traffic)

// Motor speed (0-255 for PWM)
const int MOTOR_SPEED = 255;

// Enum for the 10 states
enum State {
  STATE0,
  STATE1,
  STATE2,
  STATE3,
  STATE4,
  STATE5,
  STATE6,
  STATE7,
  STATE8,
  STATE9
};

// Current state and timer variables
State currentState = STATE0;
unsigned long stateStartTime = 0;
bool bridgeOpen = false;
bool stateActionsLogged = false;  // For servos and lights
bool motorActionLogged = false;   // For motor actions (stop, open, close)

// Servo objects
Servo servoL;
Servo servoR;

// State transition durations (ms)
const unsigned long DELAY_STATE1 = 2000;
const unsigned long DELAY_STATE2 = 2000;
const unsigned long DELAY_STATE3 = 2000;
const unsigned long DELAY_STATE4 = 5000;  // Longer for opening
const unsigned long DELAY_STATE5 = 5000;  // Longer for boat passing
const unsigned long DELAY_STATE6 = 2000;
const unsigned long DELAY_STATE7 = 2000;
const unsigned long DELAY_STATE8 = 5000;  // Longer for closing
const unsigned long DELAY_STATE9 = 2000;

// WebServer on port 80
WebServer server(80);

// Hardcoded credentials
const String adminUsername = "admin";
const String adminPassword = "admin";

// Simple token for authentication
String authToken = "";

// Define your computer's IP hosting the React app
const String REACT_APP_URL = "http://192.168.4.2:3000";  // Replace with your computer's IP

// Generate a simple token
String generateToken() {
  return "secure_token_" + String(random(100000, 999999));
}

// *** NEW: Log all headers for debugging ***
void logAllHeaders() {
  Serial.println("Received headers:");
  for (uint8_t i = 0; i < server.headers(); i++) {
    Serial.print("  ");
    Serial.print(server.headerName(i));
    Serial.print(": ");
    Serial.println(server.header(i));
  }
}

// Set all lights to LOW (off)
void resetLights() {
  digitalWrite(TRAFFIC_A_RED, LOW);
  digitalWrite(TRAFFIC_A_YELLOW, LOW);
  digitalWrite(TRAFFIC_A_GREEN, LOW);
  digitalWrite(BOAT_B_RED, LOW);
  digitalWrite(BOAT_B_YELLOW, LOW);
  digitalWrite(BOAT_B_GREEN, LOW);
}

// DC Motor control functions
void motorStop() {
  analogWrite(MOTOR_SPEED_PIN, 0);
  if (!motorActionLogged) {
    Serial.println("Motor: Stopped");
    motorActionLogged = true;
  }
}

void motorOpen() {  // Forward to open bridge
  digitalWrite(MOTOR_DIRECTION_PIN, HIGH);
  analogWrite(MOTOR_SPEED_PIN, MOTOR_SPEED);
  if (!motorActionLogged) {
    Serial.println("Motor: Opening bridge");
    motorActionLogged = true;
  }
}

void motorClose() {  // Reverse to close bridge
  digitalWrite(MOTOR_DIRECTION_PIN, LOW);
  analogWrite(MOTOR_SPEED_PIN, MOTOR_SPEED);
  if (!motorActionLogged) {
    Serial.println("Motor: Closing bridge");
    motorActionLogged = true;
  }
}

// Servo (boom gate) control functions
void boomGateRaise() {  // Raise boom (traffic can pass)
  if (!servoL.attached()) servoL.attach(SERVO_L_PIN);
  if (!servoR.attached()) servoR.attach(SERVO_R_PIN);
  delay(50);  // Short settle time
  servoL.write(SERVO_RAISED_ANGLE);
  servoR.write(SERVO_RAISED_ANGLE);
  if (!stateActionsLogged) {
    Serial.println("Boom Gate: Raised (traffic pass)");
  }
}

void boomGateLower() {  // Lower boom (stop traffic)
  if (!servoL.attached()) servoL.attach(SERVO_L_PIN);
  if (!servoR.attached()) servoR.attach(SERVO_R_PIN);
  delay(50);  // Short settle time
  servoL.write(SERVO_LOWERED_ANGLE);
  servoR.write(SERVO_LOWERED_ANGLE);
  if (!stateActionsLogged) {
    Serial.println("Boom Gate: Lowered (stop traffic)");
  }
}

void boomGateHold() {  // Detach to stop PWM and reduce jitter
  servoL.detach();
  servoR.detach();
  if (!stateActionsLogged) {
    Serial.println("Boom Gate: Detached (holding, no jitter)");
  }
}

// Set lights and actuators based on current state (no motor calls here)
void setLightsForState(State state) {
  resetLights();
  boomGateHold();  // Servos hold (detached by default)

  switch (state) {
    case STATE0:
      digitalWrite(TRAFFIC_A_GREEN, HIGH);
      digitalWrite(BOAT_B_RED, HIGH);
      boomGateRaise();  // Raise gate (traffic OK)
      break;
    case STATE1:
      digitalWrite(TRAFFIC_A_YELLOW, HIGH);
      digitalWrite(BOAT_B_RED, HIGH);
      boomGateRaise();  // Still raised (preparing)
      break;
    case STATE2:
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(BOAT_B_RED, HIGH);
      boomGateRaise();  // Raised, but red light stops traffic
      break;
    case STATE3:
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(BOAT_B_RED, HIGH);
      boomGateRaise();  // Raised, hold for open
      break;
    case STATE4:
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(BOAT_B_YELLOW, HIGH);
      boomGateLower();  // Lower boom gate to block traffic
      break;
    case STATE5:
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(BOAT_B_GREEN, HIGH);
      // Boom gate remains lowered
      break;
    case STATE6:
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(BOAT_B_YELLOW, HIGH);
      // Boom gate remains lowered
      break;
    case STATE7:
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(BOAT_B_RED, HIGH);
      // Boom gate remains lowered
      break;
    case STATE8:
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(BOAT_B_RED, HIGH);
      boomGateRaise();  // Raise boom gate as bridge closes
      break;
    case STATE9:
      digitalWrite(TRAFFIC_A_RED, HIGH);
      // digitalWrite(TRAFFIC_A_YELLOW, HIGH);  // Uncomment if needed
      digitalWrite(BOAT_B_RED, HIGH);
      boomGateRaise();  // Ensure raised when closed
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

// Add CORS headers
void addCorsHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type, x-auth-token, X-Auth-Token, X-AUTH-TOKEN");
}

// Handle OPTIONS requests
void handleOptions() {
  Serial.println("Handling OPTIONS request");
  addCorsHeaders();
  server.send(204);
}

// Handle login POST
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

// Handle state GET
void handleState() {
  Serial.println("Handling GET /api/state");
  addCorsHeaders();
  logAllHeaders();  // Log all headers for debugging
  String token = server.header("x-auth-token");
  if (token == "") {
    token = server.header("X-Auth-Token");
  }
  if (token == "") {
    token = server.header("X-AUTH-TOKEN");
  }
  if (token == "") {
    // Fallback: Check query parameter
    token = server.arg("token");
    Serial.println("No header token, checking query param token: '" + token + "'");
  }
  Serial.print("State request token received: '");
  Serial.print(token);
  Serial.println("'");
  Serial.print("Expected authToken: '");
  Serial.print(authToken);
  Serial.println("'");
  if (token != authToken || authToken == "") {
    Serial.println("State request failed: Unauthorized");
    server.send(401, "application/json", "{\"error\":\"Unauthorized\"}");
    return;
  }
  DynamicJsonDocument doc(1024);
  doc["currentState"] = currentState;
  doc["bridgeState"] = bridgeOpen;
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

// Handle command POST
void handleCommand() {
  Serial.println("Handling POST /api/command");
  addCorsHeaders();
  logAllHeaders();
  String token = server.header("x-auth-token");
  if (token == "") {
    token = server.header("X-Auth-Token");
  }
  if (token == "") {
    token = server.header("X-AUTH-TOKEN");
  }
  if (token == "") {
    token = server.arg("token");
    Serial.println("No header token, checking query param token: '" + token + "'");
  }
  Serial.print("Command request token received: '");
  Serial.print(token);
  Serial.println("'");
  Serial.print("Expected authToken: '");
  Serial.print(authToken);
  Serial.println("'");
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
    if (action == "open") {
      if (currentState == STATE0) {
        Serial.println("Command: Open bridge");
        currentState = STATE1;
        stateActionsLogged = false;
        motorActionLogged = false;
        setLightsForState(currentState);
        stateStartTime = millis();
        // Remove bridgeOpen = true; // Moved to transition in loop
        digitalWrite(LED_PIN, HIGH);
        Serial.println("LED: HIGH");
      }
    } else if (action == "close") {
      if (bridgeOpen) {
        Serial.println("Command: Close bridge");
        currentState = STATE6;
        stateActionsLogged = false;
        motorActionLogged = false;
        setLightsForState(currentState);
        stateStartTime = millis();
        // Remove bridgeOpen = false; // Moved to transition in loop
        digitalWrite(LED_PIN, LOW);
        Serial.println("LED: LOW");
      }
    } else if (action == "clear") {
      Serial.println("Command: Clear");
      currentState = STATE0;
      stateActionsLogged = false;
      motorActionLogged = false;
      setLightsForState(currentState);
      stateStartTime = millis();
      bridgeOpen = false;
      digitalWrite(LED_PIN, LOW);
      Serial.println("LED: LOW");
    }
    server.send(200, "application/json", "{\"success\":true}");
  } else {
    Serial.println("Command failed: No body");
    server.send(400, "application/json", "{\"error\":\"No body\"}");
  }
}

// Handle logout GET
void handleLogout() {
  Serial.println("Handling GET /api/logout");
  addCorsHeaders();
  logAllHeaders();
  String token = server.header("x-auth-token");
  if (token == "") {
    token = server.header("X-Auth-Token");
  }
  if (token == "") {
    token = server.header("X-AUTH-TOKEN");
  }
  if (token == "") {
    token = server.arg("token");
    Serial.println("No header token, checking query param token: '" + token + "'");
  }
  Serial.print("Logout request token received: '");
  Serial.print(token);
  Serial.println("'");
  Serial.print("Expected authToken: '");
  Serial.print(authToken);
  Serial.println("'");
  if (token != authToken || authToken == "") {
    Serial.println("Logout request failed: Unauthorized");
    server.send(401, "application/json", "{\"error\":\"Unauthorized\"}");
    return;
  }
  authToken = "";
  server.send(200, "application/json", "{\"success\":true}");
}

// Handle root path (/)
void handleRoot() {
  Serial.println("Handling GET /");
  addCorsHeaders();
  logAllHeaders();
  String token = server.header("x-auth-token");
  Serial.print("Root request token received: '");
  Serial.print(token);
  Serial.println("'");
  server.sendHeader("Location", REACT_APP_URL);
  server.send(302, "text/plain", "Redirecting to React app...");
}

void setup() {
  randomSeed(analogRead(34));  // GPIO34 is ADC1_CH6, change if used

  // Pin modes for lights (OUTPUT)
  pinMode(TRAFFIC_A_RED, OUTPUT);
  pinMode(TRAFFIC_A_YELLOW, OUTPUT);
  pinMode(TRAFFIC_A_GREEN, OUTPUT);
  pinMode(BOAT_B_RED, OUTPUT);
  pinMode(BOAT_B_YELLOW, OUTPUT);
  pinMode(BOAT_B_GREEN, OUTPUT);

  // Other pins
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);

  // Motor pins (OUTPUT)
  pinMode(MOTOR_DIRECTION_PIN, OUTPUT);
  pinMode(MOTOR_SPEED_PIN, OUTPUT);

  // Initial setup
  setLightsForState(currentState);
  motorStop();
  stateStartTime = millis();
  bridgeOpen = false;

  // Attach servos
  servoL.attach(SERVO_L_PIN);
  servoR.attach(SERVO_R_PIN);
  boomGateRaise();

  digitalWrite(LED_PIN, HIGH);
  Serial.println("LED: HIGH (startup)");
  delay(500);
  digitalWrite(LED_PIN, LOW);
  Serial.println("LED: LOW (startup)");

  Serial.begin(115200);
  Serial.println("Starting ESP32 with DC Motor and Boom Gate Servos...");
  Serial.println("Servos configured as boom gates: 0°=raised (pass), 90°=lowered (stop).");
  Serial.println("Motor runs continuously in STATE4 (open) and STATE8 (close).");

  WiFi.softAP(ssid, ap_password);
  delay(500);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  // API routes
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
}

unsigned long lastButtonPress = 0;  // For improved debounce

void loop() {
  server.handleClient();

  // Improved button debounce
  if (digitalRead(BUTTON_PIN) == LOW && millis() - lastButtonPress > 200) {
    lastButtonPress = millis();
    if (currentState == STATE0) {
      Serial.println("Button pressed: Ship detected, transitioning to STATE1");
      currentState = STATE1;
      stateActionsLogged = false;
      motorActionLogged = false;
      setLightsForState(currentState);
      stateStartTime = millis();
      // Remove bridgeOpen = true; // Moved to transition in loop
      digitalWrite(LED_PIN, HIGH);
      Serial.println("LED: HIGH");
    }
  }

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
        bridgeOpen = true;  // Add: Start animation when entering STATE3 ("Pending Open")
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
        bridgeOpen = false;  // Add: Start closing animation when entering STATE8 ("Bridge Closing")
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
        // Remove bridgeOpen = false; // Already set when entering STATE8
        digitalWrite(LED_PIN, LOW);
        Serial.println("LED: LOW");
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
        Serial.println("LED: LOW");
      }
      break;
  }
}

