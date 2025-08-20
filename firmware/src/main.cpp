#include <WiFi.h>
#include <ESPAsyncWebServer.h>

// Network credentials for Access Point
const char* ssid = "esp56";
const char* password = "12345678";

// Hardcoded credentials (in production, hash these!)
const char* validUsername = "admin";
const char* validPassword = "admin";

// GPIO pins
const int bridgePin = 2; // Bridge control
const int buttonPin = 34; // Physical button
const int redLedPin = 12;
const int yellowLedPin = 26;
const int greenLedPin = 32;

// Button debouncing
int lastButtonState = HIGH; // Initial state (not pressed, pull-up)
int bridgeState = LOW; // Initial bridge state (closed)
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50; // 50ms debounce

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// HTML login page
const char login_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP32 Bridge Login</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta http-equiv="Cache-Control" content="no-cache, no-store, must-revalidate">
  <meta http-equiv="Pragma" content="no-cache">
  <meta http-equiv="Expires" content="0">
  <style>
    html {font-family: Arial, sans-serif; text-align: center;}
    h1 {font-size: 1.8rem; color: white; background: #333; padding: 10px;}
    .form {margin: 20px auto; width: 200px;}
    input {margin: 5px; padding: 8px; width: 100%; font-size: 1rem;}
    .button {width: 100%; padding: 10px; background: #4CAF50; color: white; border: none; border-radius: 5px; cursor: pointer;}
  </style>
</head>
<body>
  <h1>ESP32 Bridge Login</h1>
  <form action="/login" method="POST" class="form">
    <input type="text" name="username" placeholder="Username" required><br>
    <input type="password" name="password" placeholder="Password" required><br>
    <input type="submit" value="Login" class="button">
  </form>
  <script>
    console.log("Login page loaded at " + new Date());
  </script>
</body>
</html>)rawliteral";

// Enhanced SVG control page with LED controls and state polling
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP32 Bridge Control</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta http-equiv="Cache-Control" content="no-cache, no-store, must-revalidate">
  <meta http-equiv="Pragma" content="no-cache">
  <meta http-equiv="Expires" content="0">
  <style>
    html {font-family: Arial, sans-serif; text-align: center;}
    body {max-width: 600px; margin: 0 auto; padding: 10px; background: #f0f0f0;}
    h2 {font-size: 2rem; color: #333;}
    .status {font-size: 1.2rem; margin: 20px; color: #555;}
    .button {padding: 12px 24px; margin: 10px; font-size: 1rem; border: none; border-radius: 5px; cursor: pointer; transition: background 0.3s;}
    .button.open {background: #4CAF50; color: white;}
    .button.close {background: #F44336; color: white;}
    .button.logout {background: #888; color: white;}
    .button.red {background: #F44336; color: white;}
    .button.yellow {background: #FFCA28; color: black;}
    .button.green {background: #4CAF50; color: white;}
    .button:hover {opacity: 0.9;}
    .bridge-container {width: 300px; height: 200px; margin: 20px auto; background: #e0f7fa; border: 1px solid #ccc; position: relative;}
    .bridge {transition: transform 3s ease-in-out;}
    .bridge.open {transform: translateY(-50px);}
    .bridge.closed {transform: translateY(0);}
    .bridge.opening {animation: pulse 1s infinite;}
    .bridge.closing {animation: pulse 1s infinite;}
    @keyframes pulse {
      0%% {opacity: 1;}
      50%% {opacity: 0.5;}
      100%% {opacity: 1;}
    }
    .fallback {color: red; font-size: 1rem; margin-top: 10px;}
    .led-controls {margin-top: 20px;}
    @media (max-width: 400px) {
      .bridge-container {width: 250px; height: 180px;}
      .button {padding: 10px 20px; font-size: 0.9rem;}
    }
  </style>
</head>
<body>
  <h2>ESP32 Bridge Control</h2>
  <button class="button logout" id="logoutButton">Logout</button>
  <div class="status">Bridge Status: <span id="state" data-state="%STATE%"></span></div>
  <div class="bridge-container" id="bridgeContainer">
    <svg width="300" height="200" viewBox="0 0 300 200">
      <!-- Water background -->
      <path d="M0 180 C50 170, 100 190, 150 180 C200 170, 250 190, 300 180 L300 200 L0 200 Z" fill="#4fc3f7"/>
      <!-- Left tower -->
      <rect x="50" y="50" width="20" height="130" fill="#666"/>
      <!-- Right tower -->
      <rect x="230" y="50" width="20" height="130" fill="#666"/>
      <!-- Bridge deck -->
      <rect x="70" y="100" width="160" height="20" fill="#888" id="bridge" class="bridge %STATE_CLASS%"/>
      <!-- Tower cables -->
      <line x1="60" y1="50" x2="90" y2="100" stroke="#999" stroke-width="3"/>
      <line x1="60" y1="50" x2="110" y2="100" stroke="#999" stroke-width="3"/>
      <line x1="240" y1="50" x2="210" y2="100" stroke="#999" stroke-width="3"/>
      <line x1="240" y1="50" x2="190" y2="100" stroke="#999" stroke-width="3"/>
    </svg>
    <p class="fallback" id="fallback" style="display:none;">Failed to load bridge graphic</p>
  </div>
  <button class="button open" id="openButton">Open Bridge</button>
  <button class="button close" id="closeButton">Close Bridge</button>
  <div class="led-controls">
    <button class="button red" id="redLedButton" data-pin="12" data-state="0">Turn Red LED On</button>
    <button class="button yellow" id="yellowLedButton" data-pin="26" data-state="0">Turn Yellow LED On</button>
    <button class="button green" id="greenLedButton" data-pin="32" data-state="0">Turn Green LED On</button>
  </div>
<script>
document.addEventListener("DOMContentLoaded", function() {
  try {
    console.log("Control page loaded at " + new Date());
    const stateElement = document.getElementById("state");
    const bridgeElement = document.getElementById("bridge");
    const openButton = document.getElementById("openButton");
    const closeButton = document.getElementById("closeButton");
    const logoutButton = document.getElementById("logoutButton");
    const redLedButton = document.getElementById("redLedButton");
    const yellowLedButton = document.getElementById("yellowLedButton");
    const greenLedButton = document.getElementById("greenLedButton");
    if (!stateElement || !bridgeElement || !openButton || !closeButton || !logoutButton || !redLedButton || !yellowLedButton || !greenLedButton) {
      console.error("Required elements not found: state=" + !!stateElement + ", bridge=" + !!bridgeElement + ", openButton=" + !!openButton + ", closeButton=" + !!closeButton + ", logoutButton=" + !!logoutButton + ", redLedButton=" + !!redLedButton + ", yellowLedButton=" + !!yellowLedButton + ", greenLedButton=" + !!greenLedButton);
      document.getElementById("fallback").style.display = "block";
      return;
    }
    const initialState = stateElement.getAttribute("data-state") || "Closed";
    const initialStateClass = bridgeElement.getAttribute("class") || "bridge closed";
    console.log("Initial bridge state: " + initialState);
    console.log("Initial bridge class: " + initialStateClass);
    stateElement.innerHTML = initialState;
    bridgeElement.setAttribute("class", initialStateClass);
    console.log("Initial transform: " + getComputedStyle(bridgeElement).transform);

    openButton.addEventListener("click", function() {
      console.log("Open button clicked at " + new Date());
      controlBridge('open');
    });
    closeButton.addEventListener("click", function() {
      console.log("Close button clicked at " + new Date());
      controlBridge('close');
    });
    logoutButton.addEventListener("click", function() {
      console.log("Logout button clicked at " + new Date());
      logoutButtonFn();
    });

    redLedButton.addEventListener("click", function() {
      toggleLed(redLedButton);
    });
    yellowLedButton.addEventListener("click", function() {
      toggleLed(yellowLedButton);
    });
    greenLedButton.addEventListener("click", function() {
      toggleLed(greenLedButton);
    });

    function controlBridge(action) {
      console.log("controlBridge called with action: " + action);
      try {
        stateElement.innerHTML = action === 'open' ? 'Opening' : 'Closing';
        bridgeElement.setAttribute("class", 'bridge ' + (action === 'open' ? 'opening' : 'closing'));
        console.log("Set bridge class to: " + bridgeElement.getAttribute("class"));
        console.log("Transform after class change: " + getComputedStyle(bridgeElement).transform);
        var xhr = new XMLHttpRequest();
        xhr.open("GET", "/update?state=" + (action === 'open' ? 1 : 0), true);
        xhr.onreadystatechange = function() {
          if (xhr.readyState === 4) {
            if (xhr.status === 200) {
              console.log("Bridge request successful, updating to " + action);
              setTimeout(() => {
                stateElement.innerHTML = action === 'open' ? 'Open' : 'Closed';
                bridgeElement.setAttribute("class", 'bridge ' + (action === 'open' ? 'open' : 'closed'));
                console.log("Bridge state updated to: " + (action === 'open' ? 'Open' : 'Closed'));
                console.log("Final bridge class: " + bridgeElement.getAttribute("class"));
                console.log("Final transform: " + getComputedStyle(bridgeElement).transform);
              }, 3000);
            } else {
              console.error("Bridge request failed, status: " + xhr.status + ", response: " + xhr.responseText);
              document.getElementById("fallback").style.display = "block";
              // Fallback: update UI
              setTimeout(() => {
                stateElement.innerHTML = action === 'open' ? 'Open' : 'Closed';
                bridgeElement.setAttribute("class", 'bridge ' + (action === 'open' ? 'open' : 'closed'));
                console.log("Fallback bridge state updated to: " + (action === 'open' ? 'Open' : 'Closed'));
                console.log("Fallback transform: " + getComputedStyle(bridgeElement).transform);
              }, 3000);
            }
          }
        };
        xhr.onerror = function() {
          console.error("Network error during /update request");
          document.getElementById("fallback").style.display = "block";
          // Fallback: update UI
          setTimeout(() => {
            stateElement.innerHTML = action === 'open' ? 'Open' : 'Closed';
            bridgeElement.setAttribute("class", 'bridge ' + (action === 'open' ? 'open' : 'closed'));
            console.log("Fallback bridge state updated to: " + (action === 'open' ? 'Open' : 'Closed'));
            console.log("Fallback transform: " + getComputedStyle(bridgeElement).transform);
          }, 3000);
        };
        xhr.send();
      } catch (e) {
        console.error("Error in controlBridge: " + e.message);
        document.getElementById("fallback").style.display = "block";
      }
    }

    function toggleLed(button) {
      const pin = button.getAttribute("data-pin");
      const currentState = button.getAttribute("data-state") === "1" ? 1 : 0;
      const newState = currentState === 1 ? 0 : 1;
      console.log("Toggling LED on pin " + pin + " to state: " + newState + " at " + new Date());
      try {
        var xhr = new XMLHttpRequest();
        xhr.open("GET", "/update-led?pin=" + pin + "&state=" + newState, true);
        xhr.onreadystatechange = function() {
          if (xhr.readyState === 4) {
            if (xhr.status === 200) {
              console.log("LED request successful, pin " + pin + " set to " + newState);
              button.setAttribute("data-state", newState);
              button.innerHTML = newState === 1 ? "Turn " + (pin === "12" ? "Red" : pin === "26" ? "Yellow" : "Green") + " LED Off" : "Turn " + (pin === "12" ? "Red" : pin === "26" ? "Yellow" : "Green") + " LED On";
            } else {
              console.error("LED request failed, status: " + xhr.status + ", response: " + xhr.responseText);
              document.getElementById("fallback").style.display = "block";
            }
          }
        };
        xhr.onerror = function() {
          console.error("Network error during /update-led request for pin " + pin);
          document.getElementById("fallback").style.display = "block";
        };
        xhr.send();
      } catch (e) {
        console.error("Error in toggleLed for pin " + pin + ": " + e.message);
        document.getElementById("fallback").style.display = "block";
      }
    }

    function logoutButtonFn() {
      console.log("logoutButtonFn called at " + new Date());
      try {
        var xhr = new XMLHttpRequest();
        xhr.open("GET", "/logout", true);
        xhr.onreadystatechange = function() {
          if (xhr.readyState === 4 && xhr.status === 200) {
            console.log("Logout request successful, redirecting");
            window.location = "/logged-out";
          } else if (xhr.readyState === 4) {
            console.error("Logout request failed, status: " + xhr.status);
          }
        };
        xhr.onerror = function() {
          console.error("Network error during /logout request");
        };
        xhr.send();
      } catch (e) {
        console.error("Error in logoutButtonFn: " + e.message);
      }
    }

    // Poll bridge state every 1 second
    setInterval(function() {
      try {
        var xhr = new XMLHttpRequest();
        xhr.open("GET", "/get-state", true);
        xhr.onreadystatechange = function() {
          if (xhr.readyState === 4 && xhr.status === 200) {
            const newState = xhr.responseText;
            console.log("Polled bridge state: " + newState);
            if (newState === "Open" || newState === "Closed") {
              if (stateElement.innerHTML !== newState) {
                stateElement.innerHTML = newState === "Open" ? "Opening" : "Closing";
                bridgeElement.setAttribute("class", 'bridge ' + (newState === "Open" ? "opening" : "closing"));
                console.log("Polled bridge class set to: " + bridgeElement.getAttribute("class"));
                console.log("Polled transform: " + getComputedStyle(bridgeElement).transform);
                setTimeout(() => {
                  stateElement.innerHTML = newState;
                  bridgeElement.setAttribute("class", 'bridge ' + (newState === "Open" ? "open" : "closed"));
                  console.log("Polled bridge state updated to: " + newState);
                  console.log("Polled final class: " + bridgeElement.getAttribute("class"));
                  console.log("Polled final transform: " + getComputedStyle(bridgeElement).transform);
                }, 3000);
              }
            }
          } else if (xhr.readyState === 4) {
            console.error("Get state request failed, status: " + xhr.status);
          }
        };
        xhr.onerror = function() {
          console.error("Network error during /get-state request");
        };
        xhr.send();
      } catch (e) {
        console.error("Error polling bridge state: " + e.message);
      }
    }, 1000);
  } catch (e) {
    console.error("Error loading control page: " + e.message);
    document.getElementById("fallback").style.display = "block";
  }
});
</script>
</body>
</html>)rawliteral";

const char logout_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP32 Bridge Logout</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta http-equiv="Cache-Control" content="no-cache, no-store, must-revalidate">
  <meta http-equiv="Pragma" content="no-cache">
  <meta http-equiv="Expires" content="0">
  <style>
    html {font-family: Arial, sans-serif; text-align: center;}
    p {font-size: 1.2rem; color: #555;}
  </style>
</head>
<body>
  <p>Logged out successfully. <a href="/">Return to login</a>.</p>
  <p><strong>Note:</strong> Close all browser tabs to complete logout.</p>
  <script>
    console.log("Logout page loaded at " + new Date());
  </script>
</body>
</html>)rawliteral";

// Authentication flag
bool isAuthenticated = false;

// Replaces placeholder with bridge state
String processor(const String& var) {
  if (var == "STATE") {
    String state = digitalRead(bridgePin) ? "Open" : "Closed";
    Serial.println("Processor: STATE = " + state);
    return state;
  }
  if (var == "STATE_CLASS") {
    String stateClass = digitalRead(bridgePin) ? "open" : "closed";
    Serial.println("Processor: STATE_CLASS = " + stateClass);
    return stateClass;
  }
  Serial.println("Processor: Ignored unknown variable: " + var);
  return var; // Return original string to avoid breaking HTML
}

void setup() {
  // Serial for debugging
  Serial.begin(115200);
  Serial.println("Starting ESP32...");

  // Initialize GPIO pins
  pinMode(bridgePin, OUTPUT);
  digitalWrite(bridgePin, LOW);
  Serial.println("Bridge GPIO 2 initialized to LOW");

  pinMode(buttonPin, INPUT_PULLUP);
  Serial.println("Button GPIO 34 initialized with pull-up");

  pinMode(redLedPin, OUTPUT);
  digitalWrite(redLedPin, LOW);
  Serial.println("Red LED GPIO 12 initialized to LOW");

  pinMode(yellowLedPin, OUTPUT);
  digitalWrite(yellowLedPin, LOW);
  Serial.println("Yellow LED GPIO 26 initialized to LOW");

  pinMode(greenLedPin, OUTPUT);
  digitalWrite(greenLedPin, LOW);
  Serial.println("Green LED GPIO 32 initialized to LOW");

  // Set up Access Point
  Serial.println("Setting up Access Point...");
  WiFi.softAP(ssid, password);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  // Route for login page (root)
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("GET / request received");
    if (!isAuthenticated) {
      Serial.println("Not authenticated, serving login page");
      Serial.printf("Login page length: %u bytes\n", strlen_P(login_html));
      request->send_P(200, "text/html", login_html);
    } else {
      Serial.println("Authenticated, redirecting to /control");
      request->redirect("/control");
    }
  });

  // Handle login form submission
  server.on("/login", HTTP_POST, [](AsyncWebServerRequest *request) {
    Serial.println("POST /login request received");
    if (request->hasParam("username", true) && request->hasParam("password", true)) {
      String username = request->getParam("username", true)->value();
      String password = request->getParam("password", true)->value();
      Serial.println("Username: " + username);
      Serial.println("Password: " + password);
      if (username == validUsername && password == validPassword) {
        isAuthenticated = true;
        Serial.println("Login successful, redirecting to /control");
        request->redirect("/control");
      } else {
        Serial.println("Login failed, unauthorized");
        request->send(401, "text/html", "<h1>Unauthorized</h1><p>Wrong credentials. <a href='/'>Try again</a>.</p>");
      }
    } else {
      Serial.println("Login failed, missing credentials");
      request->send(400, "text/html", "<h1>Bad Request</h1><p>Missing credentials.</p>");
    }
  });

  // Control page route
  server.on("/control", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!isAuthenticated) {
      Serial.println("Not authenticated, redirecting to /");
      request->redirect("/");
      return;
    }
    Serial.println("GET /control request received, serving control page");
    Serial.printf("Control page length: %u bytes\n", strlen_P(index_html));
    request->send_P(200, "text/html", index_html, processor);
  });

  // Logout route
  server.on("/logout", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("GET /logout request received");
    isAuthenticated = false;
    Serial.println("Serving logout page");
    Serial.printf("Logout page length: %u bytes\n", strlen_P(logout_html));
    request->send_P(200, "text/html", logout_html);
  });

  // Logged-out page
  server.on("/logged-out", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("GET /logged-out request received");
    Serial.printf("Logout page length: %u bytes\n", strlen_P(logout_html));
    request->send_P(200, "text/html", logout_html);
  });

  // Update bridge GPIO state
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("GET /update request received");
    if (!isAuthenticated) {
      Serial.println("Not authenticated, unauthorized");
      request->send(401, "text/html", "<h1>Unauthorized</h1><p>Please <a href='/'>login</a>.</p>");
      return;
    }
    String inputMessage;
    if (request->hasParam("state")) {
      inputMessage = request->getParam("state")->value();
      bridgeState = inputMessage.toInt();
      digitalWrite(bridgePin, bridgeState);
      Serial.println("Bridge GPIO 2 state set to: " + inputMessage);
    } else {
      inputMessage = "No message sent";
      Serial.println("No bridge state parameter received");
    }
    request->send(200, "text/plain", "OK");
  });

  // Update LED GPIO state
  server.on("/update-led", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("GET /update-led request received");
    if (!isAuthenticated) {
      Serial.println("Not authenticated, unauthorized");
      request->send(401, "text/html", "<h1>Unauthorized</h1><p>Please <a href='/'>login</a>.</p>");
      return;
    }
    String inputMessage;
    if (request->hasParam("pin") && request->hasParam("state")) {
      String pinStr = request->getParam("pin")->value();
      String stateStr = request->getParam("state")->value();
      int pin = pinStr.toInt();
      int state = stateStr.toInt();
      if (pin == 12 || pin == 26 || pin == 32) {
        digitalWrite(pin, state);
        Serial.println("LED GPIO " + pinStr + " state set to: " + stateStr);
        inputMessage = "OK";
      } else {
        inputMessage = "Invalid pin";
        Serial.println("Invalid LED pin: " + pinStr);
      }
    } else {
      inputMessage = "No pin or state sent";
      Serial.println("No LED pin or state parameter received");
    }
    request->send(200, "text/plain", inputMessage);
  });

  // Get bridge state
  server.on("/get-state", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("GET /get-state request received");
    if (!isAuthenticated) {
      Serial.println("Not authenticated, unauthorized");
      request->send(401, "text/html", "<h1>Unauthorized</h1><p>Please <a href='/'>login</a>.</p>");
      return;
    }
    String state = digitalRead(bridgePin) ? "Open" : "Closed";
    Serial.println("Sending bridge state: " + state);
    request->send(200, "text/plain", state);
  });

  // Start server
  Serial.println("Starting web server...");
  server.begin();
}

void loop() {
  // Read button state
  int buttonState = digitalRead(buttonPin);
  if (buttonState != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (buttonState == LOW && lastButtonState == HIGH) { // Button pressed
      bridgeState = !bridgeState; // Toggle state
      digitalWrite(bridgePin, bridgeState);
      Serial.println("Button pressed, bridge GPIO 2 set to: " + String(bridgeState));
    }
  }
  lastButtonState = buttonState;
}