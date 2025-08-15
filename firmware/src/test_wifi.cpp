#include <WiFi.h>
#include <WebServer.h>

// WiFi Access Point credentials
const char* ssid = "ESP32_WiFi";
const char* password = "12345678";

// Button pin
const int buttonPin = 5; // GPIO 5 (D5)

// Web server on port 80
WebServer server(80);

// Variable to store the message
String message = "Hello World";

void handleRoot() {
  // HTML with AJAX for live updates
  String html = "<!DOCTYPE html><html>";
  html += "<head><title>ESP32 Button Demo</title>";
  html += "<script>";
  html += "function updateMessage() {";
  html += "  fetch('/message').then(response => response.text()).then(data => {";
  html += "    document.getElementById('message').innerText = data;";
  html += "  });";
  html += "}";
  html += "setInterval(updateMessage, 500);"; // Update every 500ms
  html += "</script></head>";
  html += "<body><h1>ESP32 Button Demo</h1>";
  html += "<p>Message: <span id='message'>" + message + "</span></p>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleMessage() {
  // Send the current message as plain text
  server.send(200, "text/plain", message);
}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  delay(1000);

  // Set up button pin with internal pull-up resistor
  pinMode(buttonPin, INPUT_PULLUP);

  // Set up ESP32 as Access Point
  Serial.println("Setting up ESP32 as Access Point...");
  WiFi.softAP(ssid, password);

  // Print IP address
  Serial.println("Access Point started!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Set up web server routes
  server.on("/", handleRoot);         // Serve the main webpage
  server.on("/message", handleMessage); // Serve the current message
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  // Handle client requests
  server.handleClient();

  // Read button state
  if (digitalRead(buttonPin) == LOW) { // Button pressed (LOW due to pull-up)
    message = "Hello Engineer";
    Serial.println("Button pressed: Hello Engineer");
  } else {
    message = "Hello World";
    Serial.println("Button released: Hello World");
  }
}