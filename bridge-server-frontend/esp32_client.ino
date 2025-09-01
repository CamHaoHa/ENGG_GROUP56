#include <WiFi.h>
#include <HTTPClient.h>

// WiFi credentials (connect to existing network)
const char *ssid = "Optus_554483"; // Replace with your 2.4GHz WiFi SSID
const char *password = "stetschaffDVEXq"; // Verify password

// External server URL
const char *serverUrl = "http://192.168.0.19:3000"; // Ubuntu server IP

// GPIO pins
const int bridgePin = 2; // Bridge control
const int buttonPin = 26; // Button
const int redLedPin = 27; // Red LED
const int yellowLedPin = 14; // Yellow LED (flashes on ship detection)
const int greenLedPin = 12; // Green LED
const int flashLedPin = 23; // Flashing LED (on ship detection)

// Button debouncing
int lastButtonState = HIGH;
int lastStableButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200; // 200ms debounce

// Ship detection
bool shipDetected = false;
bool flashing = false;
unsigned long flashStart = 0;
const unsigned long flashInterval = 500; // Blink interval (ms)
const unsigned long flashDuration = 5000; // Flash duration (ms)

// Bridge automation
unsigned long bridgeCloseDelayStart = 0;
bool bridgeClosingPending = false;
const unsigned long bridgeCloseDelay = 5000; // 5 seconds delay for bridge close

// HTTP client
HTTPClient http;

// Polling interval for sending state to server
const unsigned long stateUpdateInterval = 200; // 200ms
unsigned long lastStateUpdate = 0;

// WiFi reconnect interval
unsigned long lastWiFiReconnect = 0;
const unsigned long wifiReconnectInterval = 30000; // Try every 30 seconds

void setup() {
  Serial.begin(115200);
  delay(100); // Give Serial time to initialize
  Serial.println("Starting ESP32...");

  // Initialize GPIO pins
  Serial.println("Initializing GPIO pins...");
  pinMode(bridgePin, OUTPUT);
  digitalWrite(bridgePin, LOW);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(redLedPin, OUTPUT);
  digitalWrite(redLedPin, LOW);
  pinMode(yellowLedPin, OUTPUT);
  digitalWrite(yellowLedPin, LOW);
  pinMode(greenLedPin, OUTPUT);
  digitalWrite(greenLedPin, LOW);
  pinMode(flashLedPin, OUTPUT);
  digitalWrite(flashLedPin, LOW);
  Serial.println("GPIO pins initialized.");

  // Connect to WiFi
  connectWiFi();
}

void connectWiFi() {
  Serial.println("Connecting to WiFi: " + String(ssid));
  WiFi.begin(ssid, password);
  int wifiAttempts = 0;
  const int maxAttempts = 20; // Try for 10 seconds
  while (WiFi.status() != WL_CONNECTED && wifiAttempts < maxAttempts) {
    delay(500);
    Serial.print(".");
    Serial.print("Status: ");
    Serial.println(WiFi.status()); // Print WiFi status code
    wifiAttempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi. Status: " + String(WiFi.status()));
    Serial.println("Scanning for available networks...");
    scanNetworks();
  }
}

void scanNetworks() {
  int n = WiFi.scanNetworks();
  Serial.println("Scan complete. Found " + String(n) + " networks:");
  for (int i = 0; i < n; i++) {
    Serial.println("SSID: " + WiFi.SSID(i) + ", Signal: " + String(WiFi.RSSI(i)) + " dBm");
  }
}

void loop() {
  // Reconnect WiFi if disconnected
  if (WiFi.status() != WL_CONNECTED && millis() - lastWiFiReconnect >= wifiReconnectInterval) {
    Serial.println("WiFi disconnected, attempting to reconnect...");
    WiFi.disconnect();
    connectWiFi();
    lastWiFiReconnect = millis();
  }

  // Read button state
  int buttonState = digitalRead(buttonPin);
  if (buttonState != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (buttonState != lastStableButtonState) {
      if (buttonState == LOW && lastStableButtonState == HIGH) {
        shipDetected = true;
        flashing = true;
        flashStart = millis();
        bridgeClosingPending = false; // Cancel any pending close
        digitalWrite(bridgePin, HIGH); // Open bridge
        Serial.println("Ship detected: yes");
        sendShipState(true); // Notify server
      } else if (buttonState == HIGH && lastStableButtonState == LOW) {
        shipDetected = false;
        flashing = false;
        digitalWrite(yellowLedPin, LOW);
        digitalWrite(flashLedPin, LOW);
        bridgeClosingPending = true;
        bridgeCloseDelayStart = millis();
        Serial.println("Ship detected: no");
        sendShipState(false); // Notify server
      }
      lastStableButtonState = buttonState;
    }
  }
  lastButtonState = buttonState;

  // Handle LED flashing
  if (flashing) {
    unsigned long currentTime = millis();
    if (currentTime - flashStart >= flashDuration) {
      flashing = false;
      digitalWrite(yellowLedPin, LOW);
      digitalWrite(flashLedPin, LOW);
    } else {
      bool on = ((currentTime - flashStart) / flashInterval) % 2 == 0;
      digitalWrite(yellowLedPin, on ? HIGH : LOW);
      digitalWrite(flashLedPin, on ? HIGH : LOW);
    }
  }

  // Handle delayed bridge closing
  if (bridgeClosingPending && (millis() - bridgeCloseDelayStart >= bridgeCloseDelay)) {
    digitalWrite(bridgePin, LOW); // Close bridge
    bridgeClosingPending = false;
    sendState(); // Notify server
  }

  // Periodically send state to server and check for commands
  if (millis() - lastStateUpdate >= stateUpdateInterval && WiFi.status() == WL_CONNECTED) {
    sendState();
    checkServerCommands();
    lastStateUpdate = millis();
  }

  // Log free heap (every 10s)
  static unsigned long lastHeapCheck = 0;
  if (millis() - lastHeapCheck > 10000) {
    Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
    lastHeapCheck = millis();
  }
}

// Send ship detection state to server
void sendShipState(bool detected) {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Sending ship state to " + String(serverUrl) + "/api/ship-state");
    http.begin(String(serverUrl) + "/api/ship-state");
    http.addHeader("Content-Type", "application/json");
    String payload = "{\"shipDetected\":" + String(detected ? "true" : "false") + "}";
    int httpCode = http.POST(payload);
    if (httpCode > 0) {
      Serial.printf("Ship state sent, code: %d\n", httpCode);
    } else {
      Serial.println("Error sending ship state: " + String(http.errorToString(httpCode)));
    }
    http.end();
  } else {
    Serial.println("WiFi not connected, cannot send ship state");
  }
}

// Send bridge and LED states to server
void sendState() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Sending state to " + String(serverUrl) + "/api/state");
    http.begin(String(serverUrl) + "/api/state");
    http.addHeader("Content-Type", "application/json");
    String payload = "{\"bridgeState\":" + String(digitalRead(bridgePin) ? "true" : "false") +
                     ",\"redLed\":" + String(digitalRead(redLedPin) ? "true" : "false") +
                     ",\"yellowLed\":" + String(digitalRead(yellowLedPin) ? "true" : "false") +
                     ",\"greenLed\":" + String(digitalRead(greenLedPin) ? "true" : "false") +
                     ",\"shipDetected\":" + String(shipDetected ? "true" : "false") + "}";
    int httpCode = http.POST(payload);
    if (httpCode > 0) {
      Serial.printf("State sent, code: %d\n", httpCode);
    } else {
      Serial.println("Error sending state: " + String(http.errorToString(httpCode)));
    }
    http.end();
  } else {
    Serial.println("WiFi not connected, cannot send state");
  }
}

// Check for control commands from server
void checkServerCommands() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Checking commands from " + String(serverUrl) + "/api/commands");
    http.begin(String(serverUrl) + "/api/commands");
    int httpCode = http.GET();
    if (httpCode == 200) {
      String payload = http.getString();
      Serial.println("Received commands: " + payload);
      // Parse JSON response (simple parsing for expected format)
      if (payload.indexOf("\"bridge\":true") != -1 && !shipDetected) {
        digitalWrite(bridgePin, HIGH);
        bridgeClosingPending = false;
        Serial.println("Server command: Open bridge");
      } else if (payload.indexOf("\"bridge\":false") != -1) {
        bridgeClosingPending = true;
        bridgeCloseDelayStart = millis();
        Serial.println("Server command: Close bridge (delayed)");
      }
      if (payload.indexOf("\"redLed\":true") != -1) {
        digitalWrite(redLedPin, HIGH);
        Serial.println("Server command: Red LED on");
      } else if (payload.indexOf("\"redLed\":false") != -1) {
        digitalWrite(redLedPin, LOW);
        Serial.println("Server command: Red LED off");
      }
      if (!flashing) { // Only allow server to control yellow LED if not flashing
        if (payload.indexOf("\"yellowLed\":true") != -1) {
          digitalWrite(yellowLedPin, HIGH);
          Serial.println("Server command: Yellow LED on");
        } else if (payload.indexOf("\"yellowLed\":false") != -1) {
          digitalWrite(yellowLedPin, LOW);
          Serial.println("Server command: Yellow LED off");
        }
      }
      if (payload.indexOf("\"greenLed\":true") != -1) {
        digitalWrite(greenLedPin, HIGH);
        Serial.println("Server command: Green LED on");
      } else if (payload.indexOf("\"greenLed\":false") != -1) {
        digitalWrite(greenLedPin, LOW);
        Serial.println("Server command: Green LED off");
      }
      if (payload.indexOf("\"clearShip\":true") != -1) {
        shipDetected = false;
        flashing = false;
        digitalWrite(yellowLedPin, LOW);
        digitalWrite(flashLedPin, LOW);
        bridgeClosingPending = true;
        bridgeCloseDelayStart = millis();
        Serial.println("Server command: Clear ship");
      }
    } else {
      Serial.printf("Error fetching commands, code: %d, error: %s\n", httpCode, http.errorToString(httpCode).c_str());
    }
    http.end();
  } else {
    Serial.println("WiFi not connected, cannot fetch commands");
  }
}