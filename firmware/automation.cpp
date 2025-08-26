#include <WiFi.h>
#include <ESPAsyncWebServer.h>

// Network credentials
const char *ssid = "esp56";
const char *password = "12345678";

// Hardcoded credentials (hash in production)
const char *validUsername = "admin";
const char *validPassword = "admin";

// GPIO pins
const int bridgePin = 2;      // Bridge control
const int buttonPin = 26;     // Button
const int redLedPin = 27;     // Red LED
const int yellowLedPin = 14;  // Yellow LED (flashes on ship detection)
const int greenLedPin = 12;   // Green LED
const int flashLedPin = 23;   // Flashing LED (on ship detection)

// Button debouncing
int lastButtonState = HIGH;
int lastStableButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;  // 200ms debounce

// Ship detection
bool shipDetected = false;
bool flashing = false;
unsigned long flashStart = 0;
const unsigned long flashInterval = 500;   // Blink interval (ms)
const unsigned long flashDuration = 5000;  // Flash duration (ms)

// Bridge automation
unsigned long bridgeCloseDelayStart = 0;
bool bridgeClosingPending = false;
const unsigned long bridgeCloseDelay = 5000;  // 5 seconds delay for bridge close

// Web server on port 80
AsyncWebServer server(80);

// Authentication
bool isAuthenticated = false;

// HTML login page
const char login_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP32 Login</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html{font-family:Arial,sans-serif;text-align:center;}
    h1{font-size:1.8rem;color:#fff;background:#333;padding:10px;}
    .form{margin:20px auto;width:200px;}
    input{margin:5px;padding:8px;width:100%;font-size:1rem;}
    .button{width:100%;padding:10px;background:#4CAF50;color:#fff;border:none;border-radius:5px;cursor:pointer;}
  </style>
</head>
<body>
  <h1>ESP32 Bridge Login</h1>
  <form action="/login" method="POST" class="form">
    <input type="text" name="username" placeholder="Username" required><br>
    <input type="password" name="password" placeholder="Password" required><br>
    <input type="submit" value="Login" class="button">
  </form>
</body>
</html>)rawliteral";

// Control page with flashing ship status and delayed bridge close
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP32 Bridge</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html{font-family:Arial,sans-serif;text-align:center;}
    body{max-width:600px;margin:0 auto;padding:10px;background:#f0f0f0;}
    h2{font-size:2rem;color:#333;}
    .status{font-size:1.2rem;margin:20px;color:#555;padding:5px;}
    .button{padding:12px 24px;margin:10px;font-size:1rem;border:none;border-radius:5px;cursor:pointer;}
    .button.open{background:#4CAF50;color:#fff;}
    .button.close{background:#F44336;color:#fff;}
    .button.logout{background:#888;color:#fff;}
    .button.red{background:#F44336;color:#fff;}
    .button.yellow{background:#FFCA28;color:#000;}
    .button.green{background:#4CAF50;color:#fff;}
    .button.clear{background:#2196F3;color:#fff;}
    .bridge-container{width:300px;height:200px;margin:20px auto;background:#e0f7fa;border:1px solid #ccc;}
    .bridge{transition:transform 3s ease-in-out;}
    .bridge.open{transform:translateY(-50px);}
    .bridge.closed{transform:translateY(0);}
    .bridge.opening{animation:pulse 1s infinite;}
    .bridge.closing{animation:pulse 1s infinite;}
    @keyframes pulse{0%%{opacity:1;}50%%{opacity:0.5;}100%%{opacity:1;}}
    .fallback{color:red;font-size:1rem;margin-top:10px;}
    .ship-detected.flashing{animation:flash 0.5s infinite;border:2px solid #FFCA28;border-radius:5px;padding:8px;background:#333;}
    @keyframes flash{0%%{opacity:1;color:#FFCA28;}50%%{opacity:0.7;color:#F44336;}100%%{opacity:1;color:#FFCA28;}}
  </style>
</head>
<body>
  <h2>ESP32 Bridge Control</h2>
  <button class="button logout" id="logoutButton">Logout</button>
  <div class="status">Bridge: <span id="state">%STATE%</span></div>
  <div class="status ship-detected" id="shipStatus"><span id="shipState">%SHIP_STATE%</span></div>
  <div class="bridge-container">
    <svg width="300" height="200" viewBox="0 0 300 200">
      <path d="M0 180 C50 170,100 190,150 180 C200 170,250 190,300 180 L300 200 L0 200 Z" fill="#4fc3f7"/>
      <rect x="50" y="50" width="20" height="130" fill="#666"/>
      <rect x="230" y="50" width="20" height="130" fill="#666"/>
      <rect x="70" y="100" width="160" height="20" fill="#888" id="bridge" class="bridge %STATE_CLASS%"/>
      <line x1="60" y1="50" x2="90" y2="100" stroke="#999" stroke-width="3"/>
      <line x1="60" y1="50" x2="110" y2="100" stroke="#999" stroke-width="3"/>
      <line x1="240" y1="50" x2="210" y2="100" stroke="#999" stroke-width="3"/>
      <line x1="240" y1="50" x2="190" y2="100" stroke="#999" stroke-width="3"/>
    </svg>
    <p class="fallback" id="fallback" style="display:none;">Failed to load graphic</p>
  </div>
  <button class="button open" id="openButton">Open</button>
  <button class="button close" id="closeButton">Close</button>
  <button class="button clear" id="clearShipButton">Clear Ship</button>
  <div>
    <button class="button red" id="redLedButton" data-pin="27" data-state="0">Red LED On</button>
    <button class="button yellow" id="yellowLedButton" data-pin="14" data-state="0">Yellow LED On</button>
    <button class="button green" id="greenLedButton" data-pin="12" data-state="0">Green LED On</button>
  </div>
<script>
const state = document.getElementById("state");
const shipStatus = document.getElementById("shipStatus");
const shipState = document.getElementById("shipState");
const bridge = document.getElementById("bridge");
const openBtn = document.getElementById("openButton");
const closeBtn = document.getElementById("closeButton");
const logoutBtn = document.getElementById("logoutButton");
const clearBtn = document.getElementById("clearShipButton");
const redBtn = document.getElementById("redLedButton");
const yellowBtn = document.getElementById("yellowLedButton");
const greenBtn = document.getElementById("greenLedButton");
if (!state || !shipStatus || !shipState || !bridge || !openBtn || !closeBtn || !logoutBtn || !clearBtn || !redBtn || !yellowBtn || !greenBtn) {
  document.getElementById("fallback").style.display = "block";
} else {
  openBtn.addEventListener("click", () => controlBridge("open"));
  closeBtn.addEventListener("click", () => controlBridge("close"));
  logoutBtn.addEventListener("click", () => {
    fetch("/logout").then(() => window.location = "/logged-out");
  });
  clearBtn.addEventListener("click", () => {
    fetch("/clear-ship").then(() => {
      shipState.innerHTML = "None";
      shipStatus.classList.remove("flashing");
    });
  });
  redBtn.addEventListener("click", () => toggleLed(redBtn));
  yellowBtn.addEventListener("click", () => toggleLed(yellowBtn));
  greenBtn.addEventListener("click", () => toggleLed(greenBtn));

  function controlBridge(action) {
    state.innerHTML = action === "open" ? "Opening" : "Closing";
    bridge.setAttribute("class", "bridge " + (action === "open" ? "opening" : "closing"));
    fetch("/update?state=" + (action === "open" ? 1 : 0)).then(res => {
      if (res.ok) {
        setTimeout(() => {
          state.innerHTML = action === "open" ? "Open" : "Closed";
          bridge.setAttribute("class", "bridge " + (action === "open" ? "open" : "closed"));
        }, 3000);
      }
    });
  }

  function toggleLed(btn) {
    const pin = btn.getAttribute("data-pin");
    const state = btn.getAttribute("data-state") === "1" ? 1 : 0;
    const newState = state === 1 ? 0 : 1;
    fetch("/update-led?pin=" + pin + "&state=" + newState).then(res => {
      if (res.ok) {
        btn.setAttribute("data-state", newState);
        btn.innerHTML = newState === 1 ? "Turn " + (pin === "27" ? "Red" : pin === "14" ? "Yellow" : "Green") + " LED Off" : "Turn " + (pin === "27" ? "Red" : pin === "14" ? "Yellow" : "Green") + " LED On";
      }
    });
  }

  function checkShipState() {
    fetch("/notify-ship").then(res => res.text()).then(data => {
      shipState.innerHTML = data;
      if (data === "Detected") {
        shipStatus.classList.add("flashing");
        controlBridge("open"); // Trigger open on ship detection
      } else {
        shipStatus.classList.remove("flashing");
      }
    });
  }

  function checkBridgeAction() {
    fetch("/get-bridge-action").then(res => res.text()).then(data => {
      if (data === "close" && state.innerHTML !== "Closed") {
        controlBridge("close");
      }
    });
  }

  setInterval(() => {
    fetch("/get-state").then(res => res.text()).then(data => {
      if (data === "Open" || data === "Closed") {
        if (state.innerHTML !== data) {
          state.innerHTML = data === "Open" ? "Opening" : "Closing";
          bridge.setAttribute("class", "bridge " + (data === "Open" ? "opening" : "closing"));
          setTimeout(() => {
            state.innerHTML = data;
            bridge.setAttribute("class", "bridge " + (data === "Open" ? "open" : "closed"));
          }, 3000);
        }
      }
    });
  }, 200); // Fast polling for bridge state

  setInterval(() => {
    fetch("/get-ship-state").then(res => res.text()).then(data => {
      shipState.innerHTML = data;
      if (data === "Detected") {
        shipStatus.classList.add("flashing");
      } else {
        shipStatus.classList.remove("flashing");
      }
    });
  }, 500);

  setInterval(checkBridgeAction, 200); // Poll for delayed bridge close

  // Trigger immediate ship state check on page load
  checkShipState();
}
</script>
</body>
</html>)rawliteral";

// Logout page
const char logout_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP32 Logout</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html{font-family:Arial,sans-serif;text-align:center;}
    p{font-size:1.2rem;color:#555;}
  </style>
</head>
<body>
  <p>Logged out. <a href="/">Login</a>.</p>
</body>
</html>)rawliteral";

// Processor for HTML placeholders
String processor(const String &var) {
  if (var == "STATE") return digitalRead(bridgePin) ? "Open" : "Closed";
  if (var == "STATE_CLASS") return digitalRead(bridgePin) ? "open" : "closed";
  if (var == "SHIP_STATE") return shipDetected ? "Detected" : "None";
  return String();
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting ESP32...");
  Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
  Serial.printf("PSRAM: %s, Free PSRAM: %u bytes\n", psramFound() ? "Yes" : "No", ESP.getFreePsram());

  // Initialize GPIO pins
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

  // Set up Access Point
  WiFi.softAP(ssid, password);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  // Routes
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!isAuthenticated) {
      request->send_P(200, "text/html", login_html);
    } else {
      request->redirect("/control");
    }
  });

  server.on("/login", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("username", true) && request->hasParam("password", true)) {
      String username = request->getParam("username", true)->value();
      String password = request->getParam("password", true)->value();
      if (username == validUsername && password == validPassword) {
        isAuthenticated = true;
        request->redirect("/control");
      } else {
        request->send(401, "text/html", "<h1>Unauthorized</h1><p>Wrong credentials. <a href='/'>Try again</a>.</p>");
      }
    } else {
      request->send(400, "text/html", "<h1>Bad Request</h1><p>Missing credentials.</p>");
    }
  });

  server.on("/control", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!isAuthenticated) {
      request->redirect("/");
      return;
    }
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/logout", HTTP_GET, [](AsyncWebServerRequest *request) {
    isAuthenticated = false;
    request->send_P(200, "text/html", logout_html);
  });

  server.on("/logged-out", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", logout_html);
  });

  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!isAuthenticated) {
      request->send(401, "text/html", "<h1>Unauthorized</h1><p><a href='/'>Login</a>.</p>");
      return;
    }
    if (request->hasParam("state")) {
      digitalWrite(bridgePin, request->getParam("state")->value().toInt());
    }
    request->send(200, "text/plain", "OK");
  });

  server.on("/update-led", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!isAuthenticated) {
      request->send(401, "text/html", "<h1>Unauthorized</h1><p><a href='/'>Login</a>.</p>");
      return;
    }
    if (request->hasParam("pin") && request->hasParam("state")) {
      int pin = request->getParam("pin")->value().toInt();
      int state = request->getParam("state")->value().toInt();
      if (pin == 27 || pin == 14 || pin == 12) {
        if (pin == 14 && flashing) {
          request->send(200, "text/plain", "Yellow LED locked during ship detection");
        } else {
          digitalWrite(pin, state);
          request->send(200, "text/plain", "OK");
        }
      } else {
        request->send(200, "text/plain", "Invalid pin");
      }
    } else {
      request->send(200, "text/plain", "No pin/state");
    }
  });

  server.on("/get-state", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!isAuthenticated) {
      request->send(401, "text/html", "<h1>Unauthorized</h1><p><a href='/'>Login</a>.</p>");
      return;
    }
    request->send(200, "text/plain", digitalRead(bridgePin) ? "Open" : "Closed");
  });

  server.on("/get-ship-state", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!isAuthenticated) {
      request->send(401, "text/html", "<h1>Unauthorized</h1><p><a href='/'>Login</a>.</p>");
      return;
    }
    request->send(200, "text/plain", shipDetected ? "Detected" : "None");
  });

  server.on("/notify-ship", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!isAuthenticated) {
      request->send(401, "text/html", "<h1>Unauthorized</h1><p><a href='/'>Login</a>.</p>");
      return;
    }
    request->send(200, "text/plain", shipDetected ? "Detected" : "None");
  });

  server.on("/get-bridge-action", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!isAuthenticated) {
      request->send(401, "text/html", "<h1>Unauthorized</h1><p><a href='/'>Login</a>.</p>");
      return;
    }
    if (bridgeClosingPending && (millis() - bridgeCloseDelayStart >= bridgeCloseDelay)) {
      request->send(200, "text/plain", "close");
    } else {
      request->send(200, "text/plain", "none");
    }
  });

  server.on("/clear-ship", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!isAuthenticated) {
      request->send(401, "text/html", "<h1>Unauthorized</h1><p><a href='/'>Login</a>.</p>");
      return;
    }
    shipDetected = false;
    flashing = false;
    digitalWrite(yellowLedPin, LOW);
    digitalWrite(flashLedPin, LOW);
    bridgeClosingPending = true;
    bridgeCloseDelayStart = millis();
    request->send(200, "text/plain", "OK");
  });

  server.begin();
}

void loop() {
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
        bridgeClosingPending = false;   // Cancel any pending close
        digitalWrite(bridgePin, HIGH);  // Open bridge
        Serial.println("ship detected: yes");
      } else if (buttonState == HIGH && lastStableButtonState == LOW) {
        shipDetected = false;
        flashing = false;
        digitalWrite(yellowLedPin, LOW);
        digitalWrite(flashLedPin, LOW);
        bridgeClosingPending = true;
        bridgeCloseDelayStart = millis();
        Serial.println("ship detected: no");
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
    digitalWrite(bridgePin, LOW);  // Close bridge after 5 seconds
    bridgeClosingPending = false;
  }

  // Log free heap (every 10s)
  static unsigned long lastHeapCheck = 0;
  if (millis() - lastHeapCheck > 10000) {
    Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
    lastHeapCheck = millis();
  }
}