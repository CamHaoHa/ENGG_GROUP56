#include <WiFi.h>
#include <WebSocketsClient.h>  // Install via Library Manager: "WebSockets by Markus Sattler"
#include <NewPing.h>           // Install via Library Manager: "NewPing by Tim Eckel"

// WiFi credentials (move to .env or hardcode for now)
const char* ssid = "Optus_554483";
const char* password = "stetschaffDVEXq";

// Server details
const char* serverHost = "192.168.0.19";  // Local server IP; update later
const int serverPort = 3000;

// Hardware pins (adjust as needed)
#define TRIGGER_PIN 5
#define ECHO_PIN 18
#define MAX_DISTANCE 200  // cm
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

// Motor/LED/Speaker pins (examples)
#define MOTOR_PWM 23  // PWM for motor speed
#define TRAFFIC_A_GREEN 2
// Add more for yellow/red, side B, speaker, etc.

WebSocketsClient webSocket;

enum State { STATE_0, STATE_1, STATE_2, STATE_3, STATE_4, STATE_5, STATE_6, STATE_7, STATE_8, STATE_9 };
State currentState = STATE_0;
bool stateChanged = true;

void setup() {
  Serial.begin(115200);
  pinMode(MOTOR_PWM, OUTPUT);
  pinMode(TRAFFIC_A_GREEN, OUTPUT);
  // Add more pinModes...

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected!");

  webSocket.begin(serverHost, serverPort, "/");
  webSocket.onEvent(webSocketEvent);  // Handle incoming commands
}

void loop() {
  webSocket.loop();

  // Read sensor
  unsigned int distance = sonar.ping_cm();

  // Basic FSM example (expand with timers, full logic)
  if (distance < 50 && currentState == STATE_0) {
    currentState = STATE_1;
    stateChanged = true;
  }
  // Actuate based on state (e.g., lights, motors, speaker)

  if (stateChanged) {
    String json = "{\"state\":" + String((int)currentState) + ", \"distance\":" + String(distance) + "}";
    webSocket.sendTXT(json);
    stateChanged = false;
  }

  delay(50);  // Adjust for loop speed
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
  if (type == WStype_TEXT) {
    // Handle overrides from server, e.g., parse JSON and set state
    Serial.printf("Received: %s\n", payload);
    // Example: if command == "raise", currentState = STATE_3;
  }
}