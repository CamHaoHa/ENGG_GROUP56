#include <WiFi.h>
#include <WebSocketsClient.h>

// WiFi credentials
const char* ssid = "Optus_554483"; // Replace with your WiFi SSID
const char* password = "stetschaffDVEXq"; // Replace with your password

// Server details
const char* serverHost = "192.168.0.19:3001"; // Update with your server IP
const int serverPort = 3000;

// GPIO pins
const int bridgePin = 2; // Bridge control
const int buttonPin = 26; // Button (simulates ship)
const int redLedPinA = 27; // Traffic Red LED
const int yellowLedPinA = 14; // Traffic Yellow LED
const int greenLedPinA = 12; // Traffic Green LED
const int redLedPinB = 19; // Boat Red LED
const int yellowLedPinB = 17; // Boat Yellow LED
const int greenLedPinB = 16; // Boat Green LED
const int speakerPin = 23; // Speaker (was flashLedPin in old code)

// Button debouncing
int lastButtonState = HIGH;
int lastStableButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;

// FSM states
enum State {
  STATE_0_DEFAULT, STATE_1_SHIP_DETECTED, STATE_2_TRAFFIC_CLEAR, STATE_3_PENDING_OPEN,
  STATE_4_BRIDGE_OPEN, STATE_5_BOAT_PASSING, STATE_6_STOPPING_BOAT, STATE_7_PENDING_CLOSE,
  STATE_8_BRIDGE_CLOSING, STATE_9_TRAFFIC_READY
};
State currentState = STATE_0_DEFAULT;
bool stateChanged = true;
unsigned long stateStartTime = 0;
const unsigned long stateTimeout = 5000; // 5s for transitions

WebSocketsClient webSocket;

void setup() {
  Serial.begin(115200);
  
  // Initialize pins
  pinMode(bridgePin, OUTPUT);
  digitalWrite(bridgePin, LOW);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(redLedPinA, OUTPUT);
  digitalWrite(redLedPinA, LOW);
  pinMode(yellowLedPinA, OUTPUT);
  digitalWrite(yellowLedPinA, LOW);
  pinMode(greenLedPinA, OUTPUT);
  digitalWrite(greenLedPinA, LOW);
  pinMode(redLedPinB, OUTPUT);
  digitalWrite(redLedPinB, LOW);
  pinMode(yellowLedPinB, OUTPUT);
  digitalWrite(yellowLedPinB, LOW);
  pinMode(greenLedPinB, OUTPUT);
  digitalWrite(greenLedPinB, LOW);
  pinMode(speakerPin, OUTPUT);
  digitalWrite(speakerPin, LOW);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi, IP: " + WiFi.localIP().toString());

  // Connect to WebSocket
  webSocket.begin(serverHost, serverPort, "/");
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  webSocket.loop();

  // Read button (simulates ship)
  int buttonState = digitalRead(buttonPin);
  if (buttonState != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (buttonState != lastStableButtonState) {
      lastStableButtonState = buttonState;
      stateChanged = true;
    }
  }
  lastButtonState = buttonState;

  // FSM logic
  unsigned long currentTime = millis();
  switch (currentState) {
    case STATE_0_DEFAULT:
      digitalWrite(greenLedPinA, HIGH);
      digitalWrite(redLedPinB, HIGH);
      digitalWrite(yellowLedPinA, LOW);
      digitalWrite(redLedPinA, LOW);
      digitalWrite(yellowLedPinB, LOW);
      digitalWrite(greenLedPinB, LOW);
      digitalWrite(speakerPin, LOW);
      digitalWrite(bridgePin, LOW); // Closed
      if (lastStableButtonState == LOW) { // Button pressed = ship detected
        currentState = STATE_1_SHIP_DETECTED;
        stateStartTime = currentTime;
        stateChanged = true;
      }
      break;
    case STATE_1_SHIP_DETECTED:
      digitalWrite(yellowLedPinA, HIGH);
      digitalWrite(greenLedPinA, LOW);
      digitalWrite(redLedPinB, HIGH);
      tone(speakerPin, 1000); // Beep
      if (currentTime - stateStartTime > stateTimeout) {
        currentState = STATE_2_TRAFFIC_CLEAR;
        stateChanged = true;
      }
      break;
    case STATE_2_TRAFFIC_CLEAR:
      digitalWrite(redLedPinA, HIGH);
      digitalWrite(yellowLedPinA, LOW);
      digitalWrite(redLedPinB, HIGH);
      noTone(speakerPin);
      if (currentTime - stateStartTime > stateTimeout) {
        currentState = STATE_3_PENDING_OPEN;
        stateChanged = true;
      }
      break;
    case STATE_3_PENDING_OPEN:
      digitalWrite(redLedPinA, HIGH);
      digitalWrite(redLedPinB, HIGH);
      tone(speakerPin, 1000);
      if (currentTime - stateStartTime > stateTimeout) {
        currentState = STATE_4_BRIDGE_OPEN;
        stateChanged = true;
      }
      break;
    case STATE_4_BRIDGE_OPEN:
      digitalWrite(redLedPinA, HIGH);
      digitalWrite(yellowLedPinB, HIGH);
      digitalWrite(bridgePin, HIGH); // Open
      noTone(speakerPin);
      if (lastStableButtonState == HIGH) { // Button released = ship passed
        currentState = STATE_5_BOAT_PASSING;
        stateChanged = true;
      }
      break;
    case STATE_5_BOAT_PASSING:
      digitalWrite(redLedPinA, HIGH);
      digitalWrite(greenLedPinB, HIGH);
      digitalWrite(yellowLedPinB, LOW);
      if (currentTime - stateStartTime > stateTimeout) {
        currentState = STATE_6_STOPPING_BOAT;
        stateChanged = true;
      }
      break;
    case STATE_6_STOPPING_BOAT:
      digitalWrite(redLedPinA, HIGH);
      digitalWrite(yellowLedPinB, HIGH);
      digitalWrite(greenLedPinB, LOW);
      tone(speakerPin, 1000);
      if (currentTime - stateStartTime > stateTimeout) {
        currentState = STATE_7_PENDING_CLOSE;
        stateChanged = true;
      }
      break;
    case STATE_7_PENDING_CLOSE:
      digitalWrite(redLedPinA, HIGH);
      digitalWrite(redLedPinB, HIGH);
      noTone(speakerPin);
      if (currentTime - stateStartTime > stateTimeout) {
        currentState = STATE_8_BRIDGE_CLOSING;
        stateChanged = true;
      }
      break;
    case STATE_8_BRIDGE_CLOSING:
      digitalWrite(redLedPinA, HIGH);
      digitalWrite(redLedPinB, HIGH);
      tone(speakerPin, 1000);
      digitalWrite(bridgePin, LOW); // Close
      if (currentTime - stateStartTime > stateTimeout) {
        currentState = STATE_9_TRAFFIC_READY;
        stateChanged = true;
      }
      break;
    case STATE_9_TRAFFIC_READY:
      digitalWrite(yellowLedPinA, HIGH);
      digitalWrite(redLedPinA, LOW);
      digitalWrite(redLedPinB, HIGH);
      noTone(speakerPin);
      if (currentTime - stateStartTime > stateTimeout) {
        currentState = STATE_0_DEFAULT;
        stateChanged = true;
      }
      break;
  }

  // Send state to server on change
  if (stateChanged && WiFi.status() == WL_CONNECTED) {
    String json = "{\"currentState\":" + String((int)currentState) +
                  ",\"bridgeState\":" + String(digitalRead(bridgePin) ? "true" : "false") +
                  ",\"redLedA\":" + String(digitalRead(redLedPinA) ? "true" : "false") +
                  ",\"yellowLedA\":" + String(digitalRead(yellowLedPinA) ? "true" : "false") +
                  ",\"greenLedA\":" + String(digitalRead(greenLedPinA) ? "true" : "false") +
                  ",\"redLedB\":" + String(digitalRead(redLedPinB) ? "true" : "false") +
                  ",\"yellowLedB\":" + String(digitalRead(yellowLedPinB) ? "true" : "false") +
                  ",\"greenLedB\":" + String(digitalRead(greenLedPinB) ? "true" : "false") + "}";
    webSocket.sendTXT(json);
    stateChanged = false;
  }

  delay(50);
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
  if (type == WStype_TEXT) {
    String cmd = String((char*)payload);
    if (cmd.indexOf("\"open\":true") != -1) {
      currentState = STATE_3_PENDING_OPEN;
      stateStartTime = millis();
      stateChanged = true;
    } else if (cmd.indexOf("\"close\":true") != -1) {
      currentState = STATE_7_PENDING_CLOSE;
      stateStartTime = millis();
      stateChanged = true;
    } else if (cmd.indexOf("\"clear\":true") != -1) {
      lastStableButtonState = HIGH; // Simulate ship cleared
      stateChanged = true;
    }
  }
}