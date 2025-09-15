#include <WiFi.h>
#include <SocketIOclient.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "Optus_554483";
const char* password = "stetschaffDVEXq";

// Server details
const char* serverHost = "192.168.0.19";
const int serverPort = 3000;

// Hardware pins (adjust as needed for your wiring)
#define BUTTON_PIN 18         // Button to simulate sensor (pull-down, active-high)
#define MOTOR_PWM 23          // PWM for motor/bridge simulation
#define TRAFFIC_A_RED 19
#define TRAFFIC_A_YELLOW 21
#define TRAFFIC_A_GREEN 2
#define BOAT_B_RED 22
#define BOAT_B_YELLOW 14
#define BOAT_B_GREEN 12

SocketIOclient socketIO;

enum State { STATE_0, STATE_1, STATE_2, STATE_3, STATE_4, STATE_5, STATE_6, STATE_7, STATE_8, STATE_9 };
State currentState = STATE_0;
bool stateChanged = true;
unsigned long stateStartTime = 0;
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 50; // ms

void setup() {
  Serial.begin(115200);

  // Initialize pins
  pinMode(BUTTON_PIN, INPUT); // Pull-down resistor externally
  pinMode(MOTOR_PWM, OUTPUT);
  pinMode(TRAFFIC_A_RED, OUTPUT);
  pinMode(TRAFFIC_A_YELLOW, OUTPUT);
  pinMode(TRAFFIC_A_GREEN, OUTPUT);
  pinMode(BOAT_B_RED, OUTPUT);
  pinMode(BOAT_B_YELLOW, OUTPUT);
  pinMode(BOAT_B_GREEN, OUTPUT);

  // Turn off all lights and motor initially
  digitalWrite(TRAFFIC_A_RED, LOW);
  digitalWrite(TRAFFIC_A_YELLOW, LOW);
  digitalWrite(TRAFFIC_A_GREEN, LOW);
  digitalWrite(BOAT_B_RED, LOW);
  digitalWrite(BOAT_B_YELLOW, LOW);
  digitalWrite(BOAT_B_GREEN, LOW);
  analogWrite(MOTOR_PWM, 0);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi! IP: " + WiFi.localIP().toString());

  socketIO.begin(serverHost, serverPort, "/socket.io/?EIO=4");
  socketIO.onEvent(socketIOEvent);
  Serial.println("Socket.IO client started");
  stateStartTime = millis(); // Initialize timer
}

void loop() {
  socketIO.loop();

  unsigned long currentTime = millis();
  bool reading = digitalRead(BUTTON_PIN);

  // Debounce button
  static bool lastReading = LOW;
  bool buttonPressed = false;
  if (reading != lastReading) {
    lastDebounceTime = currentTime;
  }
  if ((currentTime - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading == HIGH && lastReading == LOW) {
      buttonPressed = true;
      Serial.println("Button pressed detected");
    }
  }
  lastReading = reading;

  // Finite State Machine - Explicitly set ALL lights in each state
  switch (currentState) {
    case STATE_0: // Default (Bridge Closed) - Green A, Red B
      digitalWrite(TRAFFIC_A_RED, LOW);
      digitalWrite(TRAFFIC_A_YELLOW, LOW);
      digitalWrite(TRAFFIC_A_GREEN, HIGH);
      digitalWrite(BOAT_B_RED, HIGH);
      digitalWrite(BOAT_B_YELLOW, LOW);
      digitalWrite(BOAT_B_GREEN, LOW);
      analogWrite(MOTOR_PWM, 0);
      if (buttonPressed) { // Simulate ship detected
        currentState = STATE_1;
        stateChanged = true;
        stateStartTime = currentTime;
        Serial.println("Transition to State 1: Ship detected");
      }
      break;

    case STATE_1: // Ship Detected - Yellow A, Red B
      digitalWrite(TRAFFIC_A_RED, LOW);
      digitalWrite(TRAFFIC_A_YELLOW, HIGH);
      digitalWrite(TRAFFIC_A_GREEN, LOW);
      digitalWrite(BOAT_B_RED, HIGH);
      digitalWrite(BOAT_B_YELLOW, LOW);
      digitalWrite(BOAT_B_GREEN, LOW);
      analogWrite(MOTOR_PWM, 0);
      if (currentTime - stateStartTime > 5000) { // 5 seconds pending
        currentState = STATE_2;
        stateChanged = true;
        stateStartTime = currentTime;
        Serial.println("Transition to State 2: Traffic clear");
      }
      break;

    case STATE_2: // Traffic Clear - Red A, Red B
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(TRAFFIC_A_YELLOW, LOW);
      digitalWrite(TRAFFIC_A_GREEN, LOW);
      digitalWrite(BOAT_B_RED, HIGH);
      digitalWrite(BOAT_B_YELLOW, LOW);
      digitalWrite(BOAT_B_GREEN, LOW);
      analogWrite(MOTOR_PWM, 0);
      if (currentTime - stateStartTime > 5000) { // 5 seconds pending
        currentState = STATE_3;
        stateChanged = true;
        stateStartTime = currentTime;
        Serial.println("Transition to State 3: Pending to open");
      }
      break;

    case STATE_3: // Pending to Open - Red A, Red B
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(TRAFFIC_A_YELLOW, LOW);
      digitalWrite(TRAFFIC_A_GREEN, LOW);
      digitalWrite(BOAT_B_RED, HIGH);
      digitalWrite(BOAT_B_YELLOW, LOW);
      digitalWrite(BOAT_B_GREEN, LOW);
      analogWrite(MOTOR_PWM, 128); // Opening
      if (currentTime - stateStartTime > 10000) { // Simulate opening time (10s for fully open)
        currentState = STATE_4;
        stateChanged = true;
        stateStartTime = currentTime;
        Serial.println("Transition to State 4: Bridge Open");
      }
      break;

    case STATE_4: // Bridge Open - Red A, Yellow B
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(TRAFFIC_A_YELLOW, LOW);
      digitalWrite(TRAFFIC_A_GREEN, LOW);
      digitalWrite(BOAT_B_RED, LOW);
      digitalWrite(BOAT_B_YELLOW, HIGH);
      digitalWrite(BOAT_B_GREEN, LOW);
      analogWrite(MOTOR_PWM, 0); // Open, no movement
      if (currentTime - stateStartTime > 5000) { // 5 seconds pending
        currentState = STATE_5;
        stateChanged = true;
        stateStartTime = currentTime;
        Serial.println("Transition to State 5: Boat Passing");
      }
      break;

    case STATE_5: // Boat Passing - Red A, Green B
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(TRAFFIC_A_YELLOW, LOW);
      digitalWrite(TRAFFIC_A_GREEN, LOW);
      digitalWrite(BOAT_B_RED, LOW);
      digitalWrite(BOAT_B_YELLOW, LOW);
      digitalWrite(BOAT_B_GREEN, HIGH);
      analogWrite(MOTOR_PWM, 0);
      if (buttonPressed) { // Simulate ship still detected - reset timer
        stateStartTime = currentTime;
        Serial.println("Ship detected in State 5 - resetting timer");
      }
      if (currentTime - stateStartTime > 30000) { // No ship for 30 seconds
        currentState = STATE_6;
        stateChanged = true;
        stateStartTime = currentTime;
        Serial.println("Transition to State 6: Stopping Boat");
      }
      break;

    case STATE_6: // Stopping Boat - Red A, Yellow B
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(TRAFFIC_A_YELLOW, LOW);
      digitalWrite(TRAFFIC_A_GREEN, LOW);
      digitalWrite(BOAT_B_RED, LOW);
      digitalWrite(BOAT_B_YELLOW, HIGH);
      digitalWrite(BOAT_B_GREEN, LOW);
      analogWrite(MOTOR_PWM, 0);
      if (currentTime - stateStartTime > 5000) { // 5 seconds pending
        currentState = STATE_7;
        stateChanged = true;
        stateStartTime = currentTime;
        Serial.println("Transition to State 7: Pending to close");
      }
      break;

    case STATE_7: // Pending to Close - Red A, Red B
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(TRAFFIC_A_YELLOW, LOW);
      digitalWrite(TRAFFIC_A_GREEN, LOW);
      digitalWrite(BOAT_B_RED, HIGH);
      digitalWrite(BOAT_B_YELLOW, LOW);
      digitalWrite(BOAT_B_GREEN, LOW);
      analogWrite(MOTOR_PWM, 128); // Closing
      if (currentTime - stateStartTime > 5000) { // 5 seconds pending
        currentState = STATE_8;
        stateChanged = true;
        stateStartTime = currentTime;
        Serial.println("Transition to State 8: Bridge Closing");
      }
      break;

    case STATE_8: // Bridge Closing - Red A, Red B
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(TRAFFIC_A_YELLOW, LOW);
      digitalWrite(TRAFFIC_A_GREEN, LOW);
      digitalWrite(BOAT_B_RED, HIGH);
      digitalWrite(BOAT_B_YELLOW, LOW);
      digitalWrite(BOAT_B_GREEN, LOW);
      analogWrite(MOTOR_PWM, 128); // Closing
      if (currentTime - stateStartTime > 10000) { // Simulate closing time (10s for fully closed)
        currentState = STATE_9;
        stateChanged = true;
        stateStartTime = currentTime;
        Serial.println("Transition to State 9: Traffic Ready");
      }
      break;

    case STATE_9: // Traffic Ready - Red A, Yellow A, Red B
      digitalWrite(TRAFFIC_A_RED, HIGH);
      digitalWrite(TRAFFIC_A_YELLOW, HIGH);
      digitalWrite(TRAFFIC_A_GREEN, LOW);
      digitalWrite(BOAT_B_RED, HIGH);
      digitalWrite(BOAT_B_YELLOW, LOW);
      digitalWrite(BOAT_B_GREEN, LOW);
      analogWrite(MOTOR_PWM, 0);
      if (currentTime - stateStartTime > 5000) { // 5 seconds pending
        currentState = STATE_0;
        stateChanged = true;
        stateStartTime = currentTime;
        Serial.println("Transition back to State 0: Default");
      }
      break;
  }

  // Send update to server if state changed
  if (stateChanged) {
    bool bridgeOpen = (currentState >= STATE_4 && currentState <= STATE_6);
    DynamicJsonDocument doc(256);
    doc["currentState"] = (int)currentState;
    doc["bridgeState"] = bridgeOpen;
    doc["redLedA"] = digitalRead(TRAFFIC_A_RED);
    doc["yellowLedA"] = digitalRead(TRAFFIC_A_YELLOW);
    doc["greenLedA"] = digitalRead(TRAFFIC_A_GREEN);
    doc["redLedB"] = digitalRead(BOAT_B_RED);
    doc["yellowLedB"] = digitalRead(BOAT_B_YELLOW);
    doc["greenLedB"] = digitalRead(BOAT_B_GREEN);
    String dataJson;
    serializeJson(doc, dataJson);
    String eventPayload = "[\"espUpdate\"," + dataJson + "]";
    socketIO.sendEVENT(eventPayload.c_str());
    Serial.println("Sent espUpdate: " + dataJson);
    stateChanged = false;
  }

  delay(50); // Small delay for loop stability
}

void socketIOEvent(socketIOmessageType_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case sIOtype_DISCONNECT:
      Serial.printf("[IOc] Disconnected!\n");
      break;
    case sIOtype_CONNECT:
      Serial.printf("[IOc] Connected to server!\n");
      break;
    case sIOtype_EVENT: {
      Serial.printf("[IOc] Received event: ");
      for (size_t i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
      }
      Serial.println();
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, payload, length);
      if (error) {
        Serial.printf("[IOc] JSON parse failed: %s\n", error.c_str());
        return;
      }
      String eventName = doc[0].as<String>();
      if (eventName == "command") {
        JsonObject cmd = doc[1];
        if (cmd["open"]) {
          Serial.println("[IOc] Manual open command received");
          currentState = STATE_3;
          stateChanged = true;
          stateStartTime = millis();
        } else if (cmd["close"]) {
          Serial.println("[IOc] Manual close command received");
          currentState = STATE_7;
          stateChanged = true;
          stateStartTime = millis();
        } else if (cmd["clear"]) {
          Serial.println("[IOc] Clear command received - reset to default");
          currentState = STATE_0;
          stateChanged = true;
          stateStartTime = millis();
        }
      } else if (eventName == "update") {
        Serial.println("[IOc] Received state update from server");
      }
      break;
    }
    case sIOtype_ERROR:
      Serial.printf("[IOc] Error: %s\n", payload);
      break;
    default:
      Serial.printf("[IOc] Unknown event type %d\n", type);
      break;
  }
}