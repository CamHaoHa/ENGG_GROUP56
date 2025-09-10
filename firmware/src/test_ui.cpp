#include <WiFi.h>
#include <WebServer.h>

// Replace with your Wi-Fi credentials
const char* ssid     = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// LED pins
int ledPins[] = {15, 2, 4};
bool ledStates[] = {false, false, false}; // Track ON/OFF

// Create a web server on port 80
WebServer server(80);

// HTML page for controlling LEDs
String htmlPage() {
  String page = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>ESP32 LED Control</title>";
  page += "<style>body{font-family:sans-serif;text-align:center;} button{padding:10px 20px;font-size:16px;margin:10px;}</style></head><body>";
  page += "<h1>ESP32 LED Controller</h1>";
  
  for (int i = 0; i < 3; i++) {
    page += "<p>LED on pin " + String(ledPins[i]) + " is " + (ledStates[i] ? "ON" : "OFF") + "</p>";
    page += "<a href='/toggle?led=" + String(i) + "'><button>Toggle LED " + String(ledPins[i]) + "</button></a><br>";
  }
  
  page += "</body></html>";
  return page;
}

// Handle root page
void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

// Handle LED toggle
void handleToggle() {
  if (server.hasArg("led")) {
    int ledIndex = server.arg("led").toInt();
    if (ledIndex >= 0 && ledIndex < 3) {
      ledStates[ledIndex] = !ledStates[ledIndex];
      digitalWrite(ledPins[ledIndex], ledStates[ledIndex] ? HIGH : LOW);
    }
  }
  server.sendHeader("Location", "/");
  server.send(303); // Redirect back to main page
}

void setup() {
  Serial.begin(115200);

  // Set LED pins as output
  for (int i = 0; i < 3; i++) {
    pinMode(ledPins[i], OUTPUT);
  }

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Set up server routes
  server.on("/", handleRoot);
  server.on("/toggle", handleToggle);

  // Start server
  server.begin();
}

void loop() {
  server.handleClient();
}
