#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// WiFi AP credentials
const char* ssid = "ESP32_Bridge";
const char* ap_password = "12345678";

// Pin definitions
const int BUTTON_PIN = 21;
const int LED_PIN = 23;
const int TRAFFIC_A_RED = 27;
const int TRAFFIC_A_YELLOW = 14;
const int TRAFFIC_A_GREEN = 12;
const int BOAT_B_RED = 5;
const int BOAT_B_YELLOW = 17;
const int BOAT_B_GREEN = 16;

// Enum for the 10 states
enum State {
  STATE0, STATE1, STATE2, STATE3, STATE4,
  STATE5, STATE6, STATE7, STATE8, STATE9
};

// Current state and timer variables
State currentState = STATE0;
unsigned long stateStartTime = 0;
bool bridgeOpen = false;

// State transition durations (ms)
const unsigned long DELAY_STATE1 = 2000;
const unsigned long DELAY_STATE2 = 2000;
const unsigned long DELAY_STATE3 = 2000;
const unsigned long DELAY_STATE4 = 2000;
const unsigned long DELAY_STATE5 = 2000;
const unsigned long DELAY_STATE6 = 2000;
const unsigned long DELAY_STATE7 = 2000;
const unsigned long DELAY_STATE8 = 2000;
const unsigned long DELAY_STATE9 = 2000;

// WebServer on port 80
WebServer server(80);

// Hardcoded credentials
const String adminUsername = "admin";
const String adminPassword = "admin";

// Simple token for authentication
String authToken = "";

// Generate a simple token
String generateToken() {
  return "secure_token_" + String(random(100000, 999999));
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

// Set lights based on current state
void setLightsForState(State state) {
  resetLights();
  switch (state) {
    case STATE0:
      digitalWrite(TRAFFIC_A_GREEN, HIGH);
      digitalWrite(BOAT_B_RED, HIGH);
      break;
    case STATE1:
      digitalWrite(TRAFFIC_A_YELLOW, HIGH);
      digitalWrite(BOAT_B_RED, HIGH);
      break;
    case STATE2:
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(BOAT_B_RED, HIGH);
      break;
    case STATE3:
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(BOAT_B_RED, HIGH);
      break;
    case STATE4:
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(BOAT_B_YELLOW, HIGH);
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
      break;
    case STATE9:
      digitalWrite(TRAFFIC_A_RED, HIGH);
      // digitalWrite(TRAFFIC_A_YELLOW, HIGH);
      digitalWrite(BOAT_B_RED, HIGH);
      break;
  }
}

// Add CORS headers
void addCorsHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type, x-auth-token");
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
  Serial.print("x-auth-token header: ");
  Serial.println(server.header("x-auth-token"));
  addCorsHeaders();
  // Temporary workaround: Bypass auth check for /api/state
  Serial.println("Bypassing auth check for /api/state");
  DynamicJsonDocument doc(512);
  doc["currentState"] = static_cast<int>(currentState);
  doc["bridgeState"] = bridgeOpen;
  doc["redLedA"] = digitalRead(TRAFFIC_A_RED) == HIGH;
  doc["yellowLedA"] = digitalRead(TRAFFIC_A_YELLOW) == HIGH;
  doc["greenLedA"] = digitalRead(TRAFFIC_A_GREEN) == HIGH;
  doc["redLedB"] = digitalRead(BOAT_B_RED) == HIGH;
  doc["yellowLedB"] = digitalRead(BOAT_B_YELLOW) == HIGH;
  doc["greenLedB"] = digitalRead(BOAT_B_GREEN) == HIGH;
  String json;
  serializeJson(doc, json);
  Serial.println("State response: " + json);
  server.send(200, "application/json", json);
}

// Handle command POST
void handleCommand() {
  Serial.println("Handling POST /api/command");
  Serial.print("x-auth-token header: ");
  Serial.println(server.header("x-auth-token"));
  addCorsHeaders();
  // Temporary workaround: Bypass auth check for /api/command
  Serial.println("Bypassing auth check for /api/command");
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
    Serial.println("Command action: " + action);
    if (action == "open") {
      if (currentState == STATE0) {
        currentState = STATE1;
        setLightsForState(currentState);
        stateStartTime = millis();
        digitalWrite(LED_PIN, HIGH);
        Serial.println("Command: Open bridge");
      }
    } else if (action == "close") {
      if (bridgeOpen) {
        currentState = STATE6;
        setLightsForState(currentState);
        stateStartTime = millis();
        Serial.println("Command: Close bridge");
      }
    } else if (action == "clear") {
      currentState = STATE0;
      setLightsForState(currentState);
      stateStartTime = millis();
      bridgeOpen = false;
      digitalWrite(LED_PIN, LOW);
      Serial.println("Command: Clear");
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
  Serial.print("x-auth-token header: ");
  Serial.println(server.header("x-auth-token"));
  addCorsHeaders();
  // Temporary workaround: Bypass auth check for /api/logout
  Serial.println("Bypassing auth check for /api/logout");
  authToken = "";
  server.send(200, "application/json", "{\"success\":true}");
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(TRAFFIC_A_RED, OUTPUT);
  pinMode(TRAFFIC_A_YELLOW, OUTPUT);
  pinMode(TRAFFIC_A_GREEN, OUTPUT);
  pinMode(BOAT_B_RED, OUTPUT);
  pinMode(BOAT_B_YELLOW, OUTPUT);
  pinMode(BOAT_B_GREEN, OUTPUT);

  setLightsForState(currentState);
  stateStartTime = millis();
  bridgeOpen = false;

  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);

  Serial.begin(115200);
  Serial.println("Starting ESP32...");

  WiFi.softAP(ssid, ap_password);
  delay(100);
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

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(50);
    if (digitalRead(BUTTON_PIN) == LOW && currentState == STATE0) {
      Serial.println("Button pressed: Ship detected, transitioning to STATE1");
      currentState = STATE1;
      setLightsForState(currentState);
      stateStartTime = millis();
      digitalWrite(LED_PIN, HIGH);
    }
  }

  unsigned long currentTime = millis();
  switch (currentState) {
    case STATE0:
      break;
    case STATE1:
      if (currentTime - stateStartTime >= DELAY_STATE1) {
        Serial.println("Transitioning to STATE2");
        currentState = STATE2;
        setLightsForState(currentState);
        stateStartTime = currentTime;
      }
      break;
    case STATE2:
      if (currentTime - stateStartTime >= DELAY_STATE2) {
        Serial.println("Transitioning to STATE3");
        currentState = STATE3;
        setLightsForState(currentState);
        stateStartTime = currentTime;
      }
      break;
    case STATE3:
      if (currentTime - stateStartTime >= DELAY_STATE3) {
        Serial.println("Transitioning to STATE4, bridge open");
        currentState = STATE4;
        setLightsForState(currentState);
        stateStartTime = currentTime;
        bridgeOpen = true;
      }
      break;
    case STATE4:
      if (currentTime - stateStartTime >= DELAY_STATE4) {
        Serial.println("Transitioning to STATE5");
        currentState = STATE5;
        setLightsForState(currentState);
        stateStartTime = currentTime;
      }
      break;
    case STATE5:
      if (currentTime - stateStartTime >= DELAY_STATE5) {
        Serial.println("Transitioning to STATE6");
        currentState = STATE6;
        setLightsForState(currentState);
        stateStartTime = currentTime;
      }
      break;
    case STATE6:
      if (currentTime - stateStartTime >= DELAY_STATE6) {
        Serial.println("Transitioning to STATE7");
        currentState = STATE7;
        setLightsForState(currentState);
        stateStartTime = currentTime;
      }
      break;
    case STATE7:
      if (currentTime - stateStartTime >= DELAY_STATE7) {
        Serial.println("Transitioning to STATE8");
        currentState = STATE8;
        setLightsForState(currentState);
        stateStartTime = currentTime;
      }
      break;
    case STATE8:
      if (currentTime - stateStartTime >= DELAY_STATE8) {
        Serial.println("Transitioning to STATE9, bridge closed");
        currentState = STATE9;
        setLightsForState(currentState);
        stateStartTime = currentTime;
        bridgeOpen = false;
      }
      break;
    case STATE9:
      if (currentTime - stateStartTime >= DELAY_STATE9) {
        Serial.println("Transitioning to STATE0");
        currentState = STATE0;
        setLightsForState(currentState);
        stateStartTime = currentTime;
        digitalWrite(LED_PIN, LOW);
      }
      break;
  }
  digitalWrite(LED_PIN, bridgeOpen ? HIGH : LOW);
}