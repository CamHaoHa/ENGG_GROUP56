┌─────────────────────────────────────────────────────┐
│ AUTOMATIC OPERATION CYCLE │
├─────────────────────────────────────────────────────┤
│ │
│ 1. Monitor ultrasonic sensors continuously │
│ └─> Detect boats at 30-50cm range │
│ │
│ 2. Boat detected → Start 500ms confirmation timer │
│ └─> Prevent false triggers │
│ │
│ 3. Confirmed detection → Initiate state sequence │
│ └─> Traffic lights → Yellow (warning) │
│ └─> Wait 3 seconds for vehicles to slow │
│ │
│ 4. Clear traffic area │
│ └─> Traffic lights → Red (stop) │
│ └─> Wait 5 seconds for clearance │
│ │
│ 5. Lower boom gates │
│ └─> Physical barrier activated │
│ └─> 3 second buffer period │
│ │
│ 6. Open bridge │
│ └─> Motor activates until top limit switch │
│ └─> 10 second timeout safety │
│ │
│ 7. Signal boats to pass │
│ └─> Boat lights → Yellow (3s warning) │
│ └─> Boat lights → Green (safe passage) │
│ └─> 10 second passage window │
│ │
│ 8. Close bridge │
│ └─> Boat lights → Yellow → Red │
│ └─> Motor reverses to bottom limit │
│ │
│ 9. Resume traffic │
│ └─> Raise boom gates │
│ └─> Traffic lights → Yellow → Green │
│ └─> Return to idle monitoring │
│ │
└─────────────────────────────────────────────────────┘
┌──────────────────────────────────────────────────────────┐
│                  SYSTEM ARCHITECTURE                     │
└──────────────────────────────────────────────────────────┘

┌─────────────────┐          ┌─────────────────┐
│   User Device   │          │   ESP32 Bridge  │
│  (Laptop/Phone) │◄────────►│   Controller    │
│                 │  WiFi    │                 │
│  ┌───────────┐  │          │  ┌───────────┐  │
│  │  React    │  │          │  │ Web Server│  │
│  │    UI     │  │          │  │   (Port   │  │
│  │           │  │          │  │    80)    │  │
│  └───────────┘  │          │  └───────────┘  │
│                 │          │                 │
│  Browser-based  │          │  REST API       │
│  192.168.4.2    │          │  192.168.4.1    │
└─────────────────┘          └────────┬────────┘
                                      │
                    ┌─────────────────┴─────────────────┐
                    │                                   │
           ┌────────▼────────┐               ┌─────────▼────────┐
           │   Input Layer   │               │   Output Layer   │
           │                 │               │                  │
           │ • Ultrasonic    │               │ • DC Motor       │
           │   Sensors (2x)  │               │ • Servo Motors   │
           │ • Limit         │               │   (2x)           │
           │   Swi Bridge Closed │
            │ • Traffic GREEN │
            │ • Boat RED      │
            │ • Gates UP tches (2x) │               │ • LED Traffic    │
           │                 │               │   Lights (6x)    │
           └─────────────────┘               └──────────────────┘

                       ┌─────────────────┐
            │    STATE 0:     │
            │      IDLE       │
            │                 │
            │ •     │
            └────────┬────────┘
                     │
            Boat Detected (30-50cm)
            Continuous 500ms
                     │
                     ▼
            ┌─────────────────┐
            │    STATE 1:     │
            │ BOAT DETECTED   │
            │                 │
            │ • Traffic YELLOW│
            │ • Boat RED      │
            │ • Gates UP      │
            │ Duration: 3s    │
            └────────┬────────┘
                     │
                     ▼
            ┌─────────────────┐
            │    STATE 2:     │
            │CLEARING TRAFFIC │
            │                 │
            │ • Traffic RED   │
            │ • Boat RED      │
            │ • Gates UP      │
            │ Duration: 5s    │
            └────────┬────────┘
                     │
                     ▼
            ┌─────────────────┐
            │   STATE 2B:     │
            │ TRAFFIC CLEAR   │
            │                 │
            │ • Traffic RED   │
            │ • Boat RED      │
            │ • Gates DOWN    │◄─── Boom gates lowered
            │ Duration: 3s    │
            └────────┬────────┘
                     │
                     ▼
            ┌─────────────────┐
            │    STATE 3:     │
            │ OPENING BRIDGE  │
            │                 │
            │ • Motor UP      │
            │ • Traffic RED   │
            │ • Boat RED      │
            │ • Gates DOWN    │
            └────────┬────────┘
                     │
            Top Limit Switch
            OR 10s timeout
                     │
                     ▼
            ┌─────────────────┐
            │    STATE 4:     │
            │ BRIDGE OPEN     │
            │ (YELLOW WARNING)│
            │                 │
            │ • Bridge OPEN   │
            │ • Traffic RED   │
            │ • Boat YELLOW   │◄─── Warning boats
            │ Duration: 3s    │
            └────────┬────────┘
                     │
                     ▼
            ┌─────────────────┐
            │    STATE 5:     │
            │  BRIDGE OPEN    │
            │ (BOATS PASSING) │
            │                 │
            │ • Bridge OPEN   │
            │ • Traffic RED   │
            │ • Boat GREEN    │◄─── Boats pass
            │ Duration: 10s   │
            └────────┬────────┘
                     │
                     ▼
            ┌─────────────────┐
            │    STATE 6:     │
            │ STOPPING BOATS  │
            │                 │
            │ • Bridge OPEN   │
            │ • Traffic RED   │
            │ • Boat YELLOW   │◄─── Warning closure
            │ Duration: 3s    │
            └────────┬────────┘
                     │
                     ▼
            ┌─────────────────┐
            │    STATE 7:     │
            │ CLOSING BRIDGE  │
            │                 │
            │ • Motor DOWN    │
            │ • Traffic RED   │
            │ • Boat RED      │
            │ • Gates DOWN    │
            └────────┬────────┘
                     │
           Bottom Limit Switch
            OR 10s timeout
                     │
                     ▼
            ┌─────────────────┐
            │    STATE 8:     │
            │ BRIDGE CLOSED   │
            │(PREPARING TRAFFIC)
            │                 │
            │ • Bridge CLOSED │
            │ • Traffic YELLOW│◄─── Preparing green
            │ • Boat RED      │
            │ • Gates UP      │
            │ Duration: 2s    │
            └────────┬────────┘
                     │
                     ▼
            Back to STATE 0 (IDLE)




                        ┌───────────────────────────────────────────────────────────────┐
            │                   SOFTWARE ARCHITECTURE                        │
            └───────────────────────────────────────────────────────────────┘
            
            ┌─────────────────────────────────────────────────────────────┐
            │                    FRONTEND LAYER (React)                   │
            │                                                             │
            │  ┌──────────────┐  ┌──────────────┐  ┌─────────────────┐  │
            │  │  Login.jsx   │  │   App.jsx    │  │ Components:     │  │
            │  │              │  │              │  │ • Boat Indicator│  │
            │  │ • Username   │  │ • State      │  │ • TrafficLight  │  │
            │  │ • Password   │  │   Management │  │ • Bridge Anim   │  │
            │  │ • Auth       │  │ • API Calls  │  │ • ControlPanel  │  │
            │  └──────┬───────┘  └──────┬───────┘  └─────────┬───────┘  │
            │         │                  │                     │          │
            │         └──────────────────┴─────────────────────┘          │
            │                            │                                │
            └────────────────────────────┼────────────────────────────────┘
                                         │
                                         │ HTTP/REST
                                         │ (WiFi: 192.168.4.x)
                                         │
            ┌────────────────────────────▼────────────────────────────────┐
            │                    BACKEND LAYER (ESP32)                    │
            │                                                             │
            │  ┌─────────────────────────────────────────────────────┐  │
            │  │              WebServer (Port 80)                    │  │
            │  │                                                      │  │
            │  │  REST API Endpoints:                                │  │
            │  │  ┌────────────────────────────────────────────────┐ │  │
            │  │  │ POST /api/login                                 │ │  │
            │  │  │  └─> Authenticate user, return token            │ │  │
            │  │  │                                                  │ │  │
            │  │  │ GET  /api/state?token=xxx                       │ │  │
            │  │  │  └─> Return current system state JSON           │ │  │
            │  │  │                                                  │ │  │
            │  │  │ POST /api/command                               │ │  │
            │  │  │  └─> Execute control commands                   │ │  │
            │  │  │      (open, close, lights, override)            │ │  │
            │  │  │                                                  │ │  │
            │  │  │ GET  /api/logout                                │ │  │
            │  │  │  └─> Invalidate auth token                      │ │  │
            │  │  └────────────────────────────────────────────────┘ │  │
            │  │                                                      │  │
            │  │  CORS Headers:                                       │  │
            │  │  • Access-Control-Allow-Origin: *                   │  │
            │  │  • Access-Control-Allow-Methods: GET, POST, OPTIONS │  │
            │  │  • Access-Control-Allow-Headers: ...                │  │
            │  └──────────────────┬───────────────────────────────────┘  │
            │                     │                                      │
            │  ┌──────────────────▼──────────────────────────────────┐  │
            │  │         AUTHENTICATION MODULE                       │  │
            │  │                                                      │  │
            │  │  • Token Generation: "secure_token_XXXXXX"          │  │
            │  │  • Token Validation: Check against stored token     │  │
            │  │  • Session Management: Single active session        │  │
            │  │  • Credentials: admin/admin (configurable)          │  │
            │  └──────────────────┬───────────────────────────────────┘  │
            │                     │                                      │
            │  ┌──────────────────▼──────────────────────────────────┐  │
            │  │           STATE MACHINE CONTROLLER                  │  │
            │  │                                                      │  │
            │  │  • Current State Tracking (STATE0-STATE8)           │  │
            │  │  • State Transition Logic                           │  │
            │  │  • Timer Management (state durations)               │  │
            │  │  • Mode Control (Auto vs Manual)                    │  │
            │  │  • Override Flag Management                         │  │
            │  └──────────────────┬───────────────────────────────────┘  │
            │                     │                                      │
            │         ┌───────────┴──────────┐                          │
            │         │                      │                          │
            │  ┌──────▼───────┐      ┌──────▼───────┐                  │
            │  │ Input Layer  │      │ Output Layer │                  │
            │  │              │      │              │                  │
            │  │ Sensors:     │      │ Actuators:   │                  │
            │  │ • Ultrasonic │      │ • DC Motor   │                  │
            │  │   (2x)       │      │ • Servos (2x)│                  │
            │  │ • Limit      │      │ • LEDs (6x)  │                  │
            │  │   Switches   │      │              │                  │
            │  └──────────────┘      └──────────────┘                  │
            │                                                           │
            └───────────────────────────────────────────────────────────┘
            
            ┌───────────────────────────────────────────────────────────┐
            │                   DATA FLOW DIAGRAM                        │
            └───────────────────────────────────────────────────────────┘
            
            User Action → React UI → HTTP Request → ESP32 WebServer
                                                          │
                                                          ├─> Auth Check
                                                          │   (Token Valid?)
                                                          │
                                                          ├─> State Machine
                                                          │   (Update State)
                                                          │
                                                          └─> Hardware Control
                                                              (GPIO Outputs)
            
            Sensor Input → ESP32 GPIO → State Machine → Update State
                                              │              Variables
                                              │
                                              └─> HTTP Response → React UI
                                                  (500ms polling)  (Update Display)



        ┌─────────────────────────────────────────────────────────┐
    │                 REST API SPECIFICATION                  │
    ├─────────────────────────────────────────────────────────┤
    │                                                         │
    │  POST /api/login                                        │
    │  ├─> Request Body:                                      │
    │  │   {                                                  │
    │  │     "username": "admin",                             │
    │  │     "password": "admin"                              │
    │  │   }                                                  │
    │  ├─> Success Response (200):                            │
    │  │   {                                                  │
    │  │     "success": true,                                 │
    │  │     "token": "secure_token_123456"                   │
    │  │   }                                                  │
    │  └─> Error Response (401):                              │
    │      {                                                  │
    │        "success": false,                                │
    │        "message": "Invalid credentials"                 │
    │      }                                                  │
    │                                                         │
    │  GET /api/state?token=xxx                               │
    │  ├─> Headers: x-auth-token: secure_token_123456        │
    │  ├─> Success Response (200):                            │
    │  │   {                                                  │
    │  │     "currentState": 0,                               │
    │  │     "bridgeState": false,                            │
    │  │     "manualOverride": false,                         │
    │  │     "boatDetected": false,                           │
    │  │     "boatSensor1": false,                            │
    │  │     "boatSensor2": false,                            │
    │  │     "redLedA": false,                                │
    │  │     "yellowLedA": false,                             │
    │  │     "greenLedA": true,                               │
    │  │     "redLedB": true,                                 │
    │  │     "yellowLedB": false,                             │
    │  │     "greenLedB": false,                              │
    │  │     "limitSwitchTop": false,                         │
    │  │     "limitSwitchBottom": true                        │
    │  │   }                                                  │
    │  └─> Error Response (401):                              │
    │      { "error": "Unauthorized" }                        │
    │                                                         │
    │  POST /api/command                                      │
    │  ├─> Headers: x-auth-token: secure_token_123456        │
    │  ├─> Request Body:                                      │
    │  │   { "action": "open" }                               │
    │  │   { "action": "close" }                              │
    │  │   { "action": "enableOverride" }                     │
    │  │   { "action": "disableOverride" }                    │
    │  │   { "action": "trafficRed" }                         │
    │  │   { "action": "trafficYellow" }                      │
    │  │   { "action": "trafficGreen" }                       │
    │  │   { "action": "boatRed" }                            │
    │  │   { "action": "boatYellow" }                         │
    │  │   { "action": "boatGreen" }                          │
    │  │   { "action": "clear" }                              │
    │  ├─> Success Response (200):                            │
    │  │   { "success": true }                                │
    │  └─> Error Responses:                                   │
    │      • 401: { "error": "Unauthorized" }                 │
    │      • 400: { "error": "Not in override mode" }         │
    │      • 400: { "error": "Unknown action" }               │
    │                                                         │
    │  GET /api/logout                                        │
    │  ├─> Headers: x-auth-token: secure_token_123456        │
    │  ├─> Success Response (200):                            │
    │  │   { "success": true }                                │
    │  └─> Error Response (401):                              │
    │      { "error": "Unauthorized" }                        │
    │                                                         │
    └─────────────────────────────────────────────────────────┘


        ┌─────────────────────────────────────────────────────────┐
    │              AUTHENTICATION FLOW DIAGRAM                │
    └─────────────────────────────────────────────────────────┘
    
    User Opens App
          │
          ▼
    ┌─────────────┐
    │ Check Local │      Token Exists?
    │  Storage    │────────────────────► Yes ─┐
    └─────────────┘                            │
          │ No                                 │
          │                                    │
          ▼                                    ▼
    ┌─────────────┐                   ┌──────────────┐
    │  Display    │                   │  Set         │
    │  Login Form │                   │  isAuth=true │
    └──────┬──────┘                   └──────┬───────┘
           │                                 │
           │ User enters credentials         │
           │                                 │
           ▼                                 │
    ┌─────────────┐                          │
    │ POST        │                          │
    │ /api/login  │                          │
    └──────┬──────┘                          │
           │                                 │
           ▼                                 │
    ┌─────────────┐                          │
    │  ESP32      │                          │
    │  Validates  │                          │
    │  Credentials│                          │
    └──────┬──────┘                          │
           │                                 │
        Valid? ◄─────────────────────────────┘
           │
       Yes │ No
           │  └──► Display Error
           │       "Invalid credentials"
           │
           ▼
    ┌─────────────┐
    │  Generate   │
    │  Auth Token │
    │  "secure_   │
    │  token_XXX" │
    └──────┬──────┘
           │
           ▼
    ┌─────────────┐
    │  Return     │
    │  Token to   │
    │  Frontend   │
    └──────┬──────┘
           │
           ▼
    ┌─────────────┐
    │  Store      │
    │  Token in   │
    │  localStorage│
    └──────┬──────┘
           │
           ▼
    ┌─────────────┐
    │  Set        │
    │  isAuth=true│
    └──────┬──────┘
           │
           ▼
    ┌─────────────┐
    │  Display    │
    │  Dashboard  │
    └──────┬──────┘
           │
           ▼
      Authenticated Session
           │
      Every Request:
      • Add header: x-auth-token
      • ESP32 validates token
      • If invalid: 401 response
           │
      3x 401 errors:
      • Clear localStorage
      • Force re-login



          ┌─────────────────────────────────────────────────────────┐
    │              SECURITY IMPLEMENTATION                    │
    ├─────────────────────────────────────────────────────────┤
    │                                                         │
    │  1. Token-Based Authentication                          │
    │  ┌────────────────────────────────────────────────┐   │
    │  │ • Token Format: "secure_token_XXXXXX"          │   │
    │  │ • Generation: Random 6-digit number            │   │
    │  │ • Storage: Local to ESP32 (in-memory)          │   │
    │  │ • Validation: Every API request                │   │
    │  │ • Single Session: One active token at a time   │   │
    │  │ • Invalidation: Logout or new login            │   │
    │  └────────────────────────────────────────────────┘   │
    │                                                         │
    │  2. Request Authentication                              │
    │  ┌────────────────────────────────────────────────┐   │
    │  │ All API requests (except /login) require:      │   │
    │  │                                                │   │
    │  │ HTTP Headers:                                  │   │
    │  │   x-auth-token: secure_token_123456            │   │
    │  │   OR                                           │   │
    │  │ Query Parameter:                               │   │
    │  │   ?token=secure_token_123456                   │   │
    │  │                                                │   │
    │  │ Validation Process:                            │   │
    │  │ 1. Extract token from header or query          │   │
    │  │ 2. Compare with stored authToken               │   │
    │  │ 3. If match: Process request                   │   │
    │  │ 4. If no match: Return 401 Unauthorized        │   │
    │  └────────────────────────────────────────────────┘   │
    │                                                         │
    │  3. CORS (Cross-Origin Resource Sharing)                │
    │  ┌────────────────────────────────────────────────┐   │
    │  │ Headers added to all responses:                │   │
    │  │                                                │   │
    │  │ • Access-Control-Allow-Origin: *               │   │
    │  │ • Access-Control-Allow-Methods:                │   │
    │  │   GET, POST, OPTIONS                           │   │
    │  │ • Access-Control-Allow-Headers:                │   │
    │  │   Content-Type, x-auth-token, X-Auth-Token     │   │
    │  │                                                │   │
    │  │ OPTIONS Preflight:                             │   │
    │  │ • Handled explicitly                           │   │
    │  │ • Returns 204 No Content                       │   │
    │  └────────────────────────────────────────────────┘   │
    │                                                         │
    │  4. Client-Side Session Management                      │
    │  ┌────────────────────────────────────────────────┐   │
    │  │ • Token stored in localStorage                 │   │
    │  │ • Persistent across page refreshes             │   │
    │  │ • Cleared on logout                            │   │
    │  │ • Cleared after 3x 401 errors                  │   │
    │  │ • No cookies (stateless)                       │   │
    │  └────────────────────────────────────────────────┘   │
    │                                                         │
    │  5. Error Handling & Security                           │
    │  ┌────────────────────────────────────────────────┐   │
    │  │ 401 Unauthorized:                              │   │
    │  │ • Track consecutive errors                     │   │
    │  │ • After 3 errors: Force logout                 │   │
    │  │ • Clear token and local storage                │   │
    │  │ • Redirect to login                            │   │
    │  │                                                │   │
    │  │ Invalid JSON:                                  │   │
    │  │ • Return 400 Bad Request                       │   │
    │  │ • Log error                                    │   │
    │  │                                                │   │
    │  │ Unknown Actions:                               │   │
    │  │ • Return 400 with error message                │   │
    │  └────────────────────────────────────────────────┘   │
    │                                                         │
    │  6. WiFi Network Security                               │
    │  ┌────────────────────────────────────────────────┐   │
    │  │ Access Point Configuration:                    │   │
    │  │ • SSID: "group56bridge"                        │   │
    │  │ • Password: "bridgehereweare"                  │   │
    │  │ • WPA2 encryption                              │   │
    │  │ • IP Range: 192.168.4.x                        │   │
    │  │ • Gateway: 192.168.4.1 (ESP32)                 │   │
    │  │                                                │   │
    │  │ Network Isolation:                             │   │
    │  │ • Dedicated WiFi network                       │   │
    │  │ • No internet gateway                          │   │
    │  │ • Local LAN only                               │   │
    │  └────────────────────────────────────────────────┘   │
    │                                                         │
    │  7. Input Validation                                    │
    │  ┌────────────────────────────────────────────────┐   │
    │  │ • JSON parsing with error handling             │   │
    │  │ • Action whitelist validation                  │   │
    │  │ • State boundary checks                        │   │
    │  │ • Limit switch safety checks                   │   │
    │  └────────────────────────────────────────────────┘   │
    │                                                         │
    └─────────────────────────────────────────────────────────┘


        ┌─────────────────────────────────────────────────────────┐
    │         CURRENT LIMITATIONS & IMPROVEMENTS              │
    ├─────────────────────────────────────────────────────────┤
    │                                                         │
    │  Current Limitations:                                   │
    │  • Simple random token (not cryptographically secure)   │
    │  • Hardcoded credentials (admin/admin)                  │
    │  • Single user session only                             │
    │  • No token expiration time                             │
    │  • No HTTPS/TLS encryption                              │
    │  • No rate limiting                                     │
    │  • No audit logging                                     │
    │                                                         │
    │  Recommended Improvements:                              │
    │  ┌────────────────────────────────────────────────┐   │
    │  │ 1. Use cryptographic hash for tokens           │   │
    │  │    • SHA-256 or similar                        │   │
    │  │    • Longer token strings                ┌─────────────────────────────────────────────────────────┐
    │         CURRENT LIMITATIONS & IMPROVEMENTS              │
    ├─────────────────────────────────────────────────────────┤
    │                                                         │
    │  Current Limitations:                                   │
    │  • Simple random token (not cryptographically secure)   │
    │  • Hardcoded credentials (admin/admin)                  │
    │  • Single user session only                             │
    │  • No token expiration time                             │
    │  • No HTTPS/TLS encryption                              │
    │  • No rate limiting                                     │
    │  • No audit logging                                     │
    │                                                         │
    │  Recommended Improvements:                              │
    │  ┌────────────────────────────────────────────────┐   │
    │  │ 1. Use cryptographic hash for tokens           │   │
    │  │    • SHA-256 or similar                        │   │
    │  │    • Longer token strings                


    ### 6.2 REST API Endpoints

```
┌─────────────────────────────────────────────────────────────────┐
│                      REST API STRUCTURE                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  Base URL: http://192.168.4.1/api                               │
│                                                                   │
│  ┌──────────────────────────────────────────────────┐          │
│  │  POST /api/login                                  │          │
│  ├──────────────────────────────────────────────────┤          │
│  │  Request:  { username, password }                │          │
│  │  Response: { success, token }                    │          │
│  │  Purpose:  Authenticate user, get session token  │          │
│  └──────────────────────────────────────────────────┘          │
│                                                                   │
│  ┌──────────────────────────────────────────────────┐          │
│  │  GET /api/state                                   │          │
│  ├──────────────────────────────────────────────────┤          │
│  │  Headers:  { x-auth-token: <token> }             │          │
│  │  Response: {                                      │          │
│  │    currentState: 0-8,                            │          │
│  │    bridgeState: bool,                            │          │
│  │    manualOverride: bool,                         │          │
│  │    boatDetected: bool,                           │          │
│  │    boatSensor1: bool,                            │          │
│  │    boatSensor2: bool,                            │          │
│  │    redLedA, yellowLedA, greenLedA: bool,        │          │
│  │    redLedB, yellowLedB, greenLedB: bool,        │          │
│  │    limitSwitchTop: bool,                         │          │
│  │    limitSwitchBottom: bool                       │          │
│  │  }                                                │          │
│  │  Purpose:  Get real-time system status           │          │
│  └──────────────────────────────────────────────────┘          │
│                                                                   │
│  ┌──────────────────────────────────────────────────┐          │
│  │  POST /api/command                                │          │
│  ├──────────────────────────────────────────────────┤          │
│  │  Headers:  { x-auth-token: <token> }             │          │
│  │  Request:  { action: "command" }                 │          │
│  │  Commands:                                        │          │
│  │    • "enableOverride"    - Enter manual mode     │          │
│  │    • "disableOverride"   - Return to auto mode   │          │
│  │    • "open"              - Open bridge           │          │
│  │    • "close"             - Close bridge          │          │
│  │    • "trafficRed"        - Set traffic RED       │          │
│  │    • "trafficYellow"     - Set traffic YELLOW    │          │
│  │    • "trafficGreen"      - Set traffic GREEN     │          │
│  │    • "boatRed"           - Set boat RED          │          │
│  │    • "boatYellow"        - Set boat YELLOW       │          │
│  │    • "boatGreen"         - Set boat GREEN        │          │
│  │    • "clear"             - Reset to STATE 0      │          │
│  │  Response: { success: bool }                     │          │
│  └──────────────────────────────────────────────────┘          │
│                                                                   │
│  ┌──────────────────────────────────────────────────┐          │
│  │  GET /api/logout                                  │          │
│  ├──────────────────────────────────────────────────┤          │
│  │  Headers:  { x-auth-token: <token> }             │          │
│  │  Response: { success: bool }                     │          │
│  │  Purpose:  Invalidate session token              │          │
│  └──────────────────────────────────────────────────┘          │
│                                                                   │
│  CORS Headers: Enabled for cross-origin requests                │
│  Error Handling: HTTP status codes + JSON error messages        │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
```

### 6.3 Communication Flow Diagram

```
┌────────────────────────────────────────────────────────────────┐
│              CLIENT-SERVER COMMUNICATION FLOW                   │
├────────────────────────────────────────────────────────────────┤
│                                                                  │
│   React Client                           ESP32 Server          │
│   (Browser)                              (192.168.4.1)         │
│       │                                         │               │
│       │  1. POST /api/login                    │               │
│       │  { username, password }                │               │
│       ├────────────────────────────────────────►│               │
│       │                                         │               │
│       │  2. Validate Credentials               │               │
│       │                                         │               │
│       │  3. Generate Token                     │               │
│       │     token = "secure_token_123456"      │               │
│       │                                         │               │
│       │  4. Response                            │               │
│       │  { success: true, token: "..." }       │               │
│       │◄────────────────────────────────────────┤               │
│       │                                         │               │
│       │  5. Store Token (localStorage)         │               │
│       │                                         │               │
│       │  6. Poll State (every 500ms)           │               │
│       │  GET /api/state                         │               │
│       │  Header: x-auth-token: "..."           │               │
│       ├────────────────────────────────────────►│               │
│       │                                         │               │
│       │  7. Return System State                │               │
│       │  { currentState: 0, boatDetected: true,...}            │
│       │◄────────────────────────────────────────┤               │
│       │                                         │               │
│       │  8. Update UI (React State)            │               │
│       │                                         │               │
│       │  9. User Clicks "Open Bridge"          │               │
│       │                                         │               │
│       │  10. POST /api/command                  │               │
│       │  { action: "open" }                    │               │
│       │  Header: x-auth-token: "..."           │               │
│       ├────────────────────────────────────────►│               │
│       │                                         │               │
│       │  11. Validate Token                     │               │
│       │                                         │               │
│       │  12. Execute Command                    │               │
│       │      (Trigger state transition)        │               │
│       │                                         │               │
│       │  13. Response                           │               │
│       │  { success: true }                     │               │
│       │◄────────────────────────────────────────┤               │
│       │                                         │               │
│       │  14. Continue Polling...                │               │
│       │                                         │               │
│       ▼                                         ▼               │
│                                                                  │
└────────────────────────────────────────────────────────────────┘
```

---

## 7. Software High-Level Architecture

### 7.1 System Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────────────┐
│                     SOFTWARE ARCHITECTURE LAYERS                         │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                           │
│  ┌───────────────────────────────────────────────────────────────────┐ │
│  │                    PRESENTATION LAYER (Client)                     │ │
│  ├───────────────────────────────────────────────────────────────────┤ │
│  │  • React Frontend (JavaScript/JSX)                                │ │
│  │  • Material-UI Components                                         │ │
│  │  • Framer Motion Animations                                       │ │
│  │  • Real-time State Updates (500ms polling)                        │ │
│  │  • User Input Validation                                          │ │
│  │  • Session Management (localStorage)                              │ │
│  └───────────────────────┬───────────────────────────────────────────┘ │
│                          │                                               │
│                          │ HTTP/JSON over WiFi                          │
│                          │                                               │
│  ┌───────────────────────▼───────────────────────────────────────────┐ │
│  │                    APPLICATION LAYER (Server)                      │ │
│  ├───────────────────────────────────────────────────────────────────┤ │
│  │  • ESP32 WebServer (C++)                                          │ │
│  │  • REST API Endpoints                                             │ │
│  │  • Request Routing                                                │ │
│  │  • Authentication Middleware                                       │ │
│  │  • JSON Serialization (ArduinoJson)                              │ │
│  │  • CORS Headers Management                                        │ │
│  │  • Error Handling & Logging                                       │ │
│  └───────────────────────┬───────────────────────────────────────────┘ │
│                          │                                               │
│                          │                                               │
│  ┌───────────────────────▼───────────────────────────────────────────┐ │
│  │                    BUSINESS LOGIC LAYER                           │ │
│  ├───────────────────────────────────────────────────────────────────┤ │
│  │  • State Machine Controller                                       │ │
│  │  • Boat Detection Logic                                           │ │
│  │  • Safety Validation                                              │ │
│  │  • Timer Management                                               │ │
│  │  • Mode Control (Auto/Manual)                                     │ │
│  │  • Command Processing                                             │ │
│  │  • Limit Switch Monitoring                                        │ │
│  └───────────────────────┬───────────────────────────────────────────┘ │
│                          │                                               │
│                          │                                               │
│  ┌───────────────────────▼───────────────────────────────────────────┐ │
│  │                    HARDWARE ABSTRACTION LAYER                     │ │
│  ├───────────────────────────────────────────────────────────────────┤ │
│  │  • Motor Control Functions                                        │ │
│  │  • Servo Control Functions                                        │ │
│  │  • LED/Light Control                                              │ │
│  │  • Sensor Reading Functions                                       │ │
│  │  • GPIO Pin Management                                            │ │
│  │  • PWM Generation                                                 │ │
│  └───────────────────────┬───────────────────────────────────────────┘ │
│                          │                                               │
│                          │                                               │
│  ┌───────────────────────▼───────────────────────────────────────────┐ │
│  │                    HARDWARE LAYER                                  │ │
│  ├───────────────────────────────────────────────────────────────────┤ │
│  │  • ESP32 GPIO Pins                                                │ │
│  │  • DC Motor + H-Bridge                                            │ │
│  │  • Servo Motors (x2)                                              │ │
│  │  • Ultrasonic Sensors (x2)                                        │ │
│  │  • Limit Switches (x2)                                            │ │
│  │  • RGB LEDs (x6)                                                  │ │
│  │  • Status LED (x1)                                                │ │
│  │  • Power Supply                                                    │ │
│  └───────────────────────────────────────────────────────────────────┘ │
│                                                                           │
└─────────────────────────────────────────────────────────────────────────┘


## 7.3 Data Flow Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                       DATA FLOW DIAGRAM                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌───────────────┐                                              │
│  │  User Action  │                                              │
│  │  (Click/View) │                                              │
│  └───────┬───────┘                                              │
│          │                                                        │
│          ▼                                                        │
│  ┌───────────────────┐                                          │
│  │  React Component  │                                          │
│  │  Event Handler    │                                          │
│  └───────┬───────────┘                                          │
│          │                                                        │
│          │ sendCommand() / fetchState()                         │
│          ▼                                                        │
│  ┌───────────────────┐                                          │
│  │  HTTP Request     │                                          │
│  │  (Fetch API)      │                                          │
│  └───────┬───────────┘                                          │
│          │                                                        │
│          │ WiFi (JSON)                                           │
│          ▼                                                        │
│  ┌───────────────────────────────┐                             │
│  │  ESP32 WebServer              │                             │
│  │  • Route to handler           │                             │
│  │  • Validate auth token        │                             │
│  │  • Parse JSON body            │                             │
│  └───────┬───────────────────────┘                             │
│          │                                                        │
│          ▼                                                        │
│  ┌───────────────────────────────┐                             │
│  │  Business Logic               │                             │
│  │  • Validate command           │                             │
│  │  • Check current state        │                             │
│  │  • Apply safety rules         │                             │
│  └───────┬───────────────────────┘                             │
│          │                                                        │
│          ▼                                                        │
│  ┌───────────────────────────────┐                             │
│  │  State Machine Update         │                             │
│  │  • Transition state           │                             │
│  │  • Update timers              │                             │
│  │  • Set output flags           │                             │
│  └───────┬───────────────────────┘                             │
│          │                                                        │
│          ▼                                                        │
│  ┌───────────────────────────────┐                             │
│  │  Hardware Control             │                             │
│  │  • Write GPIO pins            │                             │
│  │  • Control motor/servos       │                             │
│  │  • Update LEDs                │                             │
│  └───────┬───────────────────────┘                             │
│          │                                                        │
│          │ Read Sensors                                          │
│          ▼                                                        │
│  ┌───────────────────────────────┐                             │
│  │  Physical Hardware            │                             │
│  │  • Bridge moves               │                             │
│  │  • Lights change              │                             │
│  │  • Sensors detect             │                             │
│  └───────┬───────────────────────┘                             │
│          │                                                        │
│          │ Feedback Loop (200ms)                                │
│          ▼                                                        │
│  ┌───────────────────────────────┐                             │
│  │  Read Hardware State          │                             │
│  │  • Limit switches             │                             │
│  │  • Ultrasonic sensors         │                             │
│  │  • Current state              │                             │
│  └───────┬───────────────────────┘                             │
│          │                                                        │
│          │ JSON Response                                         │
│          ▼                                                        │
│  ┌───────────────────────────────┐                             │
│  │  HTTP Response                │                             │
│  │  (JSON state data)            │                             │
│  └───────┬───────────────────────┘                             │
│          │                                                        │
│          │ WiFi                                                  │
│          ▼                                                        │
│  ┌───────────────────────────────┐                             │
│  │  React State Update           │                             │
│  │  • Update component state     │                             │
│  │  • Trigger re-render          │                             │
│  │  • Animate UI elements        │                             │
│  └───────┬───────────────────────┘                             │
│          │                                                        │
│          ▼                                                        │
│  ┌───────────────────────────────┐                             │
│  │  UI Update                    │                             │
│  │  • Display new state          │                             │
│  │  • Update animations          │                             │
│  │  • Flash indicators           │                             │
│  └───────────────────────────────┘                             │
│                                                                   │
│  Cycle repeats every 500ms (polling)                            │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
```

---

## 8. Authentication and Cybersecurity

### 8.1 Authentication Flow

```
┌─────────────────────────────────────────────────────────────────┐
│                   AUTHENTICATION FLOW DIAGRAM                    │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│   User                  React Client              ESP32 Server  │
│    │                         │                          │        │
│    │  1. Enter credentials   │                          │        │
│    ├────────────────────────►│                          │        │
│    │                         │                          │        │
│    │                         │  2. POST /api/login      │        │
│    │                         │  { username, password }  │        │
│    │                         ├─────────────────────────►│        │
│    │                         │                          │        │
│    │                         │                          │        │
│    │                         │  3. Validate Credentials │        │
│    │                         │     if (username == "admin" &&   │
│    │                         │         password == "admin") {   │
│    │                         │       token = generateToken();   │
│    │                         │     }                             │
│    │                         │                          │        │
│    │                         │  4. Response             │        │
│    │                         │  { success: true,        │        │
│    │                         │    token: "secure_..." } │        │
│    │                         │◄─────────────────────────┤        │
│    │                         │                          │        │
│    │  5. Store token         │                          │        │
│    │     localStorage.       │                          │        │
│    │     setItem("token")    │                          │        │
│    │                         │                          │        │
│    │  6. Access dashboard    │                          │        │
│    ├────────────────────────►│                          │        │
│    │                         │                          │        │
│    │                         │  7. All subsequent       │        │
│    │                         │     requests include:    │        │
│    │                         │     Header:              │        │
│    │                         │     x-auth-token: "..."  │        │
│    │                         │                          │        │
│    │                         │  8. GET /api/state       │        │
│    │                         │  Header: x-auth-token    │        │
│    │                         ├─────────────────────────►│        │
│    │                         │                          │        │
│    │                         │  9. Validate token       │        │
│    │                         │     if (token == authToken) {   │
│    │                         │       // Allow access           │
│    │                         │     } else {                     │
│    │                         │       return 401;               │
│    │                         │     }                            │
│    │                         │                          │        │
│    │                         │  10. Return data         │        │
│    │                         │◄─────────────────────────┤        │
│    │                         │                          │        │
│    │  11. Logout             │                          │        │
│    ├────────────────────────►│                          │        │
│    │                         │  12. GET /api/logout     │        │
│    │                         ├─────────────────────────►│        │
│    │                         │                          │        │
│    │                         │  13. Invalidate token    │        │
│    │                         │      authToken = "";     │        │
│    │                         │                          │        │
│    │  14. Clear localStorage │                          │        │
│    │      Redirect to login  │                          │        │
│    │                         │                          │        │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
```
### 8.2 Security Implementation

#### 8.2.1 Token Generation

```cpp
// Secure token generation with randomization
String generateToken() {
    // Seed random number generator
    randomSeed(analogRead(34));  // Use floating analog pin for entropy
    
    // Generate 6-digit random number
    int randomNum = random(100000, 999999);
    
    // Create token string
    String token = "secure_token_" + String(randomNum);
    
    return token;
}

// Example tokens:
// "secure_token_438291"
// "secure_token_756842"
// "secure_token_192847"
```

#### 8.2.2 Authentication Middleware

```cpp
void handleState() {
    addCorsHeaders();
    
    // Extract token from multiple possible locations
    String token = server.header("x-auth-token");
    if (token == "") token = server.header("X-Auth-Token");
    if (token == "") token = server.header("X-AUTH-TOKEN");
    if (token == "") token = server.arg("token");  // Query parameter fallback
    
    // Validate token
    if (token != authToken || authToken == "") {
        server.send(401, "application/json", "{\"error\":\"Unauthorized\"}");
        return;
    }
    
    // Token is valid - proceed with request
    // ... (return system state)
}
```

#### 8.2.3 Client-Side Security

```javascript
// Token storage in React
const [authToken, setAuthToken] = useState(
    localStorage.getItem("authToken") || ""
);

// Include token in all requests
const fetchState = async () => {
    const res = await fetch(`${API_URL}/api/state`, {
        headers: { 
            "x-auth-token": authToken 
        }
    });
    
    if (res.status === 401) {
        // Unauthorized - redirect to login
        localStorage.removeItem("authToken");
        setIsAuthenticated(false);
        return;
    }
    
    const data = await res.json();
    setData(data);
};
```

### 8.3 Security Features

```
┌─────────────────────────────────────────────────────────────────┐
│                      SECURITY FEATURES                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  1. AUTHENTICATION                                               │
│     ├─ Username/password validation                              │
│     ├─ Session token generation                                  │
│     └─ Token-based API access                                    │
│                                                                   │
│  2. SESSION MANAGEMENT                                           │
│     ├─ Single active token per session                           │
│     ├─ Token invalidation on logout                              │
│     ├─ Automatic retry limit (3 failed attempts)                │
│     └─ Client-side token storage (localStorage)                 │
│                                                                   │
│  3. AUTHORIZATION                                                │
│     ├─ Token validation on every request                         │
│     ├─ 401 Unauthorized responses                                │
│     └─ Automatic session expiry handling                         │
│                                                                   │
│  4. MANUAL OVERRIDE PROTECTION                                   │
│     ├─ Confirmation dialogs for critical actions                │
│     ├─ Safety checklist before bridge operations                │
│     └─ Mode indicator (Auto/Manual) always visible              │
│                                                                   │
│  5. NETWORK SECURITY                                             │
│     ├─ WiFi AP with WPA2 password                                │
│     ├─ CORS headers for controlled access                        │
│     ├─ Input validation on all endpoints                         │
│     └─ JSON parsing error handling                               │
│                                                                   │
│  6. OPERATIONAL SECURITY                                         │
│     ├─ Limit switch safety interlocks                            │
│     ├─ State machine guards (prevent invalid transitions)       │
│     ├─ Timeout protection (prevent infinite loops)              │
│     └─ Emergency "Clear" button (reset to safe state)           │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
```

### 8.4 Credential Management

**Default Credentials:**
- Username: `admin`
- Password: `admin`

**WiFi Access Point:**
- SSID: `group56bridge`
-// filepath: /home/cam/Documents/_bridge/ENGG_GROUP56/approach3/documentation/FINAL_DESIGN_REPORT.md
# ESP32 Automated Drawbridge Control System - Final Design R



┌─────────────────────────────────────────────────────┐
│  🚤 BOAT DETECTION INDICATOR - FLASHING ANIMATION   │
├─────────────────────────────────────────────────────┤
│                                                     │
│  Visual States:                                     │
│                                                     │
│  STATE: NO DETECTION                                │
│  ┌───────────────────────────────────────────────┐ │
│  │  🌊 No Boat Detected                          │ │
│  │  Monitoring waterway (30-50cm range)          │ │
│  │  Sensor 1 ○  |  Sensor 2 ○                    │ │
│  └───────────────────────────────────────────────┘ │
│  • Gray background                                  │


│  • Static display                                   │
│  • Sensor circles inactive                          │
│                                                     │
│  STATE: BOAT DETECTED (STATE0)                      │
│  ┌───────────────────────────────────────────────┐ │
│  │  🚤 BOAT DETECTED!                            │ │
│  │  Waiting for continuous detection (500ms)...  │ │
│  │  Sensor 1 ✓  |  Sensor 2 ✓                    │ │
│  └───────────────────────────────────────────────┘ │
│  • ORANGE background (flashing)                     │
│  • Pulsing glow effect                             │
│  • Rotating radar icon                             │
│  • Scale breathing animation                        │
│  • Active sensor badges green                       │
│                                                     │
│  Animation Specifications:                          │
│  ├─> Background: #FF9800 → #F57C00 → #FF9800      │
│  ├─> Box Shadow: 20px → 40px → 20px (glow)        │
│  ├─> Scale: 1.0 → 1.05 → 1.0 (breathing)          │
│  ├─> Icon Rotation: 0° → 360° (continuous)         │
│  ├─> Duration: 0.8s per cycle                      │
│  └─> Easing: easeInOut                             │
│                                                     │
└─────────────────────────────────────────────────────┘


┌─────────────────────────────────────────────────────┐
│          BRIDGE ANIMATION SPECIFICATIONS            │
├─────────────────────────────────────────────────────┤
│                                                     │
│  Closed State:                                      │
│  • Bridge span horizontal (y=160)                   │
│  • Traffic lights GREEN                             │
│  • Boat RED                                         │
│  • Water animation continuous                       │
│                                                     │
│  Opening Animation:                                 │
│  • Duration: 6 seconds                              │
│  • Y-axis: 160 → 60 (-100px)                       │
│  • ScaleY: 1.0 → 1.1 (stretching)                  │
│  • Easing: Spring (stiffness=50, damping=25)       │
│  • X-axis wobble: 0 → -2 → 0 (realistic physics)   │
│                                                     │
│  Open State:                                        │
│  • Bridge span raised (y=60)                        │
│  • Boat appears and animates                        │
│  • Path: x=-40 → x=40 → x=-40 (6s loop)           │
│  • Traffic lights RED                               │
│  • Boat lights GREEN                                │
│                                                     │
│  Closing Animation:                                 │
│  • Duration: 6 seconds                              │
│  • Y-axis: 60 → 160 (lowering)                     │
│  • ScaleY: 1.1 → 1.0                               │
│  • Same spring physics                              │
│                                                     │
│  Water Effects:                                     │
│  • 3 layered waves (depths)                         │
│  • Continuous sine wave motion                      │
│  • Y-offset: 0 → -7 → 0 (4s cycle)                │
│  • Opacity layers: 0.9, 0.7, 0.5                   │
│                                                     │
└─────────────────────────────────────────────────────┘


┌─────────────────────────────────────────────────────┐
│              CONTROL PANEL LAYOUT                   │
├─────────────────────────────────────────────────────┤
│                                                     │
│  Automatic Mode (Default):                          │
│  ┌───────────────────────────────────────────────┐ │
│  │  [🔒 Enable Manual Override]                  │ │
│  │                                                │ │
│  │  ℹ️  Automatic Mode Active                    │ │
│  │  System is monitoring sensors and managing    │ │
│  │  bridge operations automatically              │ │
│  │                                                │ │
│  │  [Open Bridge]  [Close Bridge]  [Clear]       │ │
│  │   (disabled)     (disabled)     (enabled)     │ │
│  └───────────────────────────────────────────────┘ │
│                                                     │
│  Manual Override Mode:                              │
│  ┌───────────────────────────────────────────────┐ │
│  │  ⚠️ MANUAL OVERRIDE ACTIVE                    │ │
│  │                                                │ │
│  │  [🤖 Return to Auto Mode]                     │ │
│  │                                                │ │
│  │  🚗 Vehicle Traffic Light Control             │ │
│  │  ┌─────────┐    [🔴 Set RED]                  │ │
│  │  │ Current │    [🟡 Set YELLOW]               │ │
│  │  │ Status  │    [🟢 Set GREEN]                │ │
│  │  │ Visual  │                                  │ │
│  │  └─────────┘                                  │ │
│  │                                                │ │
│  │  ⛵ Boat Traffic Light Control                 │ │
│  │  ┌─────────┐    [🔴 Set RED]                  │ │
│  │  │ Current │    [🟡 Set YELLOW]               │ │
│  │  │ Status  │    [🟢 Set GREEN]                │ │
│  │  │ Visual  │                                  │ │
│  │  └─────────┘                                  │ │
│  │                                                │ │
│  │  [⬆️ Open Bridge]  [⬇️ Close Bridge]  [🔄 Clear] │
│  │     (enabled)        (enabled)      (enabled) │ │
│  └───────────────────────────────────────────────┘ │
│                                                     │
│  Safety Confirmation Dialog:                        │
│  ┌───────────────────────────────────────────────┐ │
│  │  ⚠️ Open Bridge - Safety Confirmation         │ │
│  │                                                │ │
│  │  Before opening the bridge, please confirm:   │ │
│  │                                                │ │
│  │  • All traffic has cleared the bridge         │ │
│  │  • Boom gates are down                        │ │
│  │  • No vehicles are approaching                │ │
│  │  • Area is safe for operation                 │ │
│  │                                                │ │
│  │  Do you want to proceed?                       │ │
│  │                                                │ │
│  │       [Cancel]    [Confirm & Proceed]          │ │
│  └───────────────────────────────────────────────┘ │
│                                                     │
└─────────────────────────────────────────────────────┘



┌───────────────────────────────────────────────────────────────┐
│                   SOFTWARE ARCHITECTURE                        │
└───────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│                    FRONTEND LAYER (React)                   │
│                                                             │
│  ┌──────────────┐  ┌──────────────┐  ┌─────────────────┐  │
│  │  Login.jsx   │  │   App.jsx    │  │ Components:     │  │
│  │              │  │              │  │ • Boat Indicator│  │
│  │ • Username   │  │ • State      │  │ • TrafficLight  │  │
│  │ • Password   │  │   Management │  │ • Bridge Anim   │  │
│  │ • Auth       │  │ • API Calls  │  │ • ControlPanel  │  │
│  └──────┬───────┘  └──────┬───────┘  └─────────┬───────┘  │
│         │                  │                     │          │
│         └──────────────────┴─────────────────────┘          │
│                            │                                │
└────────────────────────────┼────────────────────────────────┘
                             │
                             │ HTTP/REST
                             │ (WiFi: 192.168.4.x)
                             │
┌────────────────────────────▼────────────────────────────────┐
│                    BACKEND LAYER (ESP32)                    │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐  │
│  │              WebServer (Port 80)                    │  │
│  │                                                      │  │
│  │  REST API Endpoints:                                │  │
│  │  ┌────────────────────────────────────────────────┐ │  │
│  │  │ POST /api/login                                 │ │  │
│  │  │  └─> Authenticate user, return token            │ │  │
│  │  │                                                  │ │  │
│  │  │ GET  /api/state?token=xxx                       │ │  │
│  │  │  └─> Return current system state JSON           │ │  │
│  │  │                                                  │ │  │
│  │  │ POST /api/command                               │ │  │
│  │  │  └─> Execute control commands                   │ │  │
│  │  │      (open, close, lights, override)            │ │  │
│  │  │                                                  │ │  │
│  │  │ GET  /api/logout                                │ │  │
│  │  │  └─> Invalidate auth token                      │ │  │
│  │  └────────────────────────────────────────────────┘ │  │
│  │                                                      │  │
│  │  CORS Headers:                                       │  │
│  │  • Access-Control-Allow-Origin: *                   │  │
│  │  • Access-Control-Allow-Methods: GET, POST, OPTIONS │  │
│  │  • Access-Control-Allow-Headers: ...                │  │
│  └──────────────────┬───────────────────────────────────┘  │
│                     │                                      │
│  ┌──────────────────▼──────────────────────────────────┐  │
│  │         AUTHENTICATION MODULE                       │  │
│  │                                                      │  │
│  │  • Token Generation: "secure_token_XXXXXX"          │  │
│  │  • Token Validation: Check against stored token     │  │
│  │  • Session Management: Single active session        │  │
│  │  • Credentials: admin/admin (configurable)          │  │
│  └──────────────────┬───────────────────────────────────┘  │
│                     │                                      │
│  ┌──────────────────▼──────────────────────────────────┐  │
│  │           STATE MACHINE CONTROLLER                  │  │
│  │                                                      │  │
│  │  • Current State Tracking (STATE0-STATE8)           │  │
│  │  • State Transition Logic                           │  │
│  │  • Timer Management (state durations)               │  │
│  │  • Mode Control (Auto vs Manual)                    │  │
│  │  • Override Flag Management                         │  │
│  └──────────────────┬───────────────────────────────────┘  │
│                     │                                      │
│         ┌───────────┴──────────┐                          │
│         │                      │                          │
│  ┌──────▼───────┐      ┌──────▼───────┐                  │
│  │ Input Layer  │      │ Output Layer │                  │
│  │              │      │              │                  │
│  │ Sensors:     │      │ Actuators:   │                  │
│  │ • Ultrasonic │      │ • DC Motor   │                  │
│  │   (2x)       │      │ • Servos (2x)│                  │
│  │ • Limit      │      │ • LEDs (6x)  │                  │
│  │   Switches   │      │              │                  │
│  └──────────────┘      └──────────────┘                  │
│                                                           │
└───────────────────────────────────────────────────────────┘

┌───────────────────────────────────────────────────────────┐
│                   DATA FLOW DIAGRAM                        │
└───────────────────────────────────────────────────────────┘

User Action → React UI → HTTP Request → ESP32 WebServer
                                              │
                                              ├─> Auth Check
                                              │   (Token Valid?)
                                              │
                                              ├─> State Machine
                                              │   (Update State)
                                              │
                                              └─> Hardware Control
                                                  (GPIO Outputs)

Sensor Input → ESP32 GPIO → State Machine → Update State
                                  │              Variables
                                  │
                                  └─> HTTP Response → React UI
                                      (500ms polling)  (Update Display)


                                                                          ┌─────────────────────────────────────────────────────────┐
                                    │         CURRENT LIMITATIONS & IMPROVEMENTS              │
                                    ├─────────────────────────────────────────────────────────┤
                                    │                                                         │
                                    │  Current Limitations:                                   │
                                    │  • Simple random token (not cryptographically secure)   │
                                    │  • Hardcoded credentials (admin/admin)                  │
                                    │  • Single user session only                             │
                                    │  • No token expiration time                             │
                                    │  • No HTTPS/TLS encryption                              │
                                    │  • No rate limiting                                     │
                                    │  • No audit logging                                     │
                                    │                                                         │
                                    │  Recommended Improvements:                              │
                                    │  ┌────────────────────────────────────────────────┐   │
                                    │  │ 1. Use cryptographic hash for tokens           │   │
                                    │  │    • SHA-256 or similar                        │   │
                                    │  │    • Longer token strings                ┌─────────────────────────────────────────────────────────┐
                                    │         CURRENT LIMITATIONS & IMPROVEMENTS              │
                                    ├─────────────────────────────────────────────────────────┤
                                    │                                                         │
                                    │  Current Limitations:                                   │
                                    │  • Simple random token (not cryptographically secure)   │
                                    │  • Hardcoded credentials (admin/admin)                  │
                                    │  • Single user session only                             │
                                    │  • No token expiration time                             │
                                    │  • No HTTPS/TLS encryption                              │
                                    │  • No rate limiting                                     │
                                    │  • No audit logging                                     │
                                    │                                                         │
                                    │  Recommended Improvements:                              │
                                    │  ┌────────────────────────────────────────────────┐   │
                                    │  │ 1. Use cryptographic hash for tokens           │   │
                                    │  │    • SHA-256 or similar                        │   │
                                    │  │    • Longer token strings                



                                    ===========================

=================================
software high level architecture 
┌───────────────────────────────────────────────────────────────┐
│                   SOFTWARE ARCHITECTURE                        │
└───────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│                    FRONTEND LAYER (React)                   │
│                                                             │
│  ┌──────────────┐  ┌──────────────┐  ┌─────────────────┐  │
│  │  Login.jsx   │  │   App.jsx    │  │ Components:     │  │
│  │              │  │              │  │ • Boat Indicator│  │
│  │ • Username   │  │ • State      │  │ • TrafficLight  │  │
│  │ • Password   │  │   Management │  │ • Bridge Anim   │  │
│  │ • Auth       │  │ • API Calls  │  │ • ControlPanel  │  │
│  └──────┬───────┘  └──────┬───────┘  └─────────┬───────┘  │
│         │                  │                     │          │
│         └──────────────────┴─────────────────────┘          │
│                            │                                │
└────────────────────────────┼────────────────────────────────┘
                             │
                             │ HTTP/REST
                             │ (WiFi: 192.168.4.x)
                             │
┌────────────────────────────▼────────────────────────────────┐
│                    BACKEND LAYER (ESP32)                    │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐  │
│  │              WebServer (Port 80)                    │  │
│  │                                                      │  │
│  │  REST API Endpoints:                                │  │
│  │  ┌────────────────────────────────────────────────┐ │  │
│  │  │ POST /api/login                                 │ │  │
│  │  │  └─> Authenticate user, return token            │ │  │
│  │  │                                                  │ │  │
│  │  │ GET  /api/state?token=xxx                       │ │  │
│  │  │  └─> Return current system state JSON           │ │  │
│  │  │                                                  │ │  │
│  │  │ POST /api/command                               │ │  │
│  │  │  └─> Execute control commands                   │ │  │
│  │  │      (open, close, lights, override)            │ │  │
│  │  │                                                  │ │  │
│  │  │ GET  /api/logout                                │ │  │
│  │  │  └─> Invalidate auth token                      │ │  │
│  │  └────────────────────────────────────────────────┘ │  │
│  │                                                      │  │
│  │  CORS Headers:                                       │  │
│  │  • Access-Control-Allow-Origin: *                   │  │
│  │  • Access-Control-Allow-Methods: GET, POST, OPTIONS │  │
│  │  • Access-Control-Allow-Headers: ...                │  │
│  └──────────────────┬───────────────────────────────────┘  │
│                     │                                      │
│  ┌──────────────────▼──────────────────────────────────┐  │
│  │         AUTHENTICATION MODULE                       │  │
│  │                                                      │  │
│  │  • Token Generation: "secure_token_XXXXXX"          │  │
│  │  • Token Validation: Check against stored token     │  │
│  │  • Session Management: Single active session        │  │
│  │  • Credentials: admin/admin (configurable)          │  │
│  └──────────────────┬───────────────────────────────────┘  │
│                     │                                      │
│  ┌──────────────────▼──────────────────────────────────┐  │
│  │           STATE MACHINE CONTROLLER                  │  │
│  │                                                      │  │
│  │  • Current State Tracking (STATE0-STATE8)           │  │
│  │  • State Transition Logic                           │  │
│  │  • Timer Management (state durations)               │  │
│  │  • Mode Control (Auto vs Manual)                    │  │
│  │  • Override Flag Management                         │  │
│  └──────────────────┬───────────────────────────────────┘  │
│                     │                                      │
│         ┌───────────┴──────────┐                          │
│         │                      │                          │
│  ┌──────▼───────┐      ┌──────▼───────┐                  │
│  │ Input Layer  │      │ Output Layer │                  │
│  │              │      │              │                  │
│  │ Sensors:     │      │ Actuators:   │                  │
│  │ • Ultrasonic │      │ • DC Motor   │                  │
│  │   (2x)       │      │ • Servos (2x)│                  │
│  │ • Limit      │      │ • LEDs (6x)  │                  │
│  │   Switches   │      │              │                  │
│  └──────────────┘      └──────────────┘                  │
│                                                           │
└───────────────────────────────────────────────────────────┘

┌───────────────────────────────────────────────────────────┐
│                   DATA FLOW DIAGRAM                        │
└───────────────────────────────────────────────────────────┘

User Action → React UI → HTTP Request → ESP32 WebServer
                                              │
                                              ├─> Auth Check
                                              │   (Token Valid?)
                                              │
                                              ├─> State Machine
                                              │   (Update State)
                                              │
                                              └─> Hardware Control
                                                  (GPIO Outputs)

Sensor Input → ESP32 GPIO → State Machine → Update State
                                  │              Variables
                                  │
                                  └─> HTTP Response → React UI
                                      (500ms polling)  (Update Display)


api specification
┌─────────────────────────────────────────────────────────┐
│                 REST API SPECIFICATION                  │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  POST /api/login                                        │
│  ├─> Request Body:                                      │
│  │   {                                                  │
│  │     "username": "admin",                             │
│  │     "password": "admin"                              │
│  │   }                                                  │
│  ├─> Success Response (200):                            │
│  │   {                                                  │
│  │     "success": true,                                 │
│  │     "token": "secure_token_123456"                   │
│  │   }                                                  │
│  └─> Error Response (401):                              │
│      {                                                  │
│        "success": false,                                │
│        "message": "Invalid credentials"                 │
│      }                                                  │
│                                                         │
│  GET /api/state?token=xxx                               │
│  ├─> Headers: x-auth-token: secure_token_123456        │
│  ├─> Success Response (200):                            │
│  │   {                                                  │
│  │     "currentState": 0,                               │
│  │     "bridgeState": false,                            │
│  │     "manualOverride": false,                         │
│  │     "boatDetected": false,                           │
│  │     "boatSensor1": false,                            │
│  │     "boatSensor2": false,                            │
│  │     "redLedA": false,                                │
│  │     "yellowLedA": false,                             │
│  │     "greenLedA": true,                               │
│  │     "redLedB": true,                                 │
│  │     "yellowLedB": false,                             │
│  │     "greenLedB": false,                              │
│  │     "limitSwitchTop": false,                         │
│  │     "limitSwitchBottom": true                        │
│  │   }                                                  │
│  └─> Error Response (401):                              │
│      { "error": "Unauthorized" }                        │
│                                                         │
│  POST /api/command                                      │
│  ├─> Headers: x-auth-token: secure_token_123456        │
│  ├─> Request Body:                                      │
│  │   { "action": "open" }                               │
│  │   { "action": "close" }                              │
│  │   { "action": "enableOverride" }                     │
│  │   { "action": "disableOverride" }                    │
│  │   { "action": "trafficRed" }                         │
│  │   { "action": "trafficYellow" }                      │
│  │   { "action": "trafficGreen" }                       │
│  │   { "action": "boatRed" }                            │
│  │   { "action": "boatYellow" }                         │
│  │   { "action": "boatGreen" }                          │
│  │   { "action": "clear" }                              │
│  ├─> Success Response (200):                            │
│  │   { "success": true }                                │
│  └─> Error Responses:                                   │
│      • 401: { "error": "Unauthorized" }                 │
│      • 400: { "error": "Not in override mode" }         │
│      • 400: { "error": "Unknown action" }               │
│                                                         │
│  GET /api/logout                                        │
│  ├─> Headers: x-auth-token: secure_token_123456        │
│  ├─> Success Response (200):                            │
│  │   { "success": true }                                │
│  └─> Error Response (401):                              │
│      { "error": "Unauthorized" }                        │
│                                                         │
└─────────────────────────────────────────────────────────┘


authentication and cynersecurity 
authentication flow 
┌─────────────────────────────────────────────────────────┐
│              AUTHENTICATION FLOW DIAGRAM                │
└─────────────────────────────────────────────────────────┘

User Opens App
      │
      ▼
┌─────────────┐
│ Check Local │      Token Exists?
│  Storage    │────────────────────► Yes ─┐
└─────────────┘                            │
      │ No                                 │
      │                                    │
      ▼                                    ▼
┌─────────────┐                   ┌──────────────┐
│  Display    │                   │  Set         │
│  Login Form │                   │  isAuth=true │
└──────┬──────┘                   └──────┬───────┘
       │                                 │
       │ User enters credentials         │
       │                                 │
       ▼                                 │
┌─────────────┐                          │
│ POST        │                          │
│ /api/login  │                          │
└──────┬──────┘                          │
       │                                 │
       ▼                                 │
┌─────────────┐                          │
│  ESP32      │                          │
│  Validates  │                          │
│  Credentials│                          │
└──────┬──────┘                          │
       │                                 │
    Valid? ◄─────────────────────────────┘
       │
   Yes │ No
       │  └──► Display Error
       │       "Invalid credentials"
       │
       ▼
┌─────────────┐
│  Generate   │
│  Auth Token │
│  "secure_   │
│  token_XXX" │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  Return     │
│  Token to   │
│  Frontend   │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  Store      │
│  Token in   │
│  localStorage│
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  Set        │
│  isAuth=true│
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  Display    │
│  Dashboard  │
└──────┬──────┘
       │
       ▼
  Authenticated Session
       │
  Every Request:
  • Add header: x-auth-token
  • ESP32 validates token
  • If invalid: 401 response
       │
  3x 401 errors:
  • Clear localStorage
  • Force re-login


  security mechanisms

  ┌─────────────────────────────────────────────────────────┐
│              SECURITY IMPLEMENTATION                    │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  1. Token-Based Authentication                          │
│  ┌────────────────────────────────────────────────┐   │
│  │ • Token Format: "secure_token_XXXXXX"          │   │
│  │ • Generation: Random 6-digit number            │   │
│  │ • Storage: Local to ESP32 (in-memory)          │   │
│  │ • Validation: Every API request                │   │
│  │ • Single Session: One active token at a time   │   │
│  │ • Invalidation: Logout or new login            │   │
│  └────────────────────────────────────────────────┘   │
│                                                         │
│  2. Request Authentication                              │
│  ┌────────────────────────────────────────────────┐   │
│  │ All API requests (except /login) require:      │   │
│  │                                                │   │
│  │ HTTP Headers:                                  │   │
│  │   x-auth-token: secure_token_123456            │   │
│  │   OR                                           │   │
│  │ Query Parameter:                               │   │
│  │   ?token=secure_token_123456                   │   │
│  │                                                │   │
│  │ Validation Process:                            │   │
│  │ 1. Extract token from header or query          │   │
│  │ 2. Compare with stored authToken               │   │
│  │ 3. If match: Process request                   │   │
│  │ 4. If no match: Return 401 Unauthorized        │   │
│  └────────────────────────────────────────────────┘   │
│                                                         │
│  3. CORS (Cross-Origin Resource Sharing)                │
│  ┌────────────────────────────────────────────────┐   │
│  │ Headers added to all responses:                │   │
│  │                                                │   │
│  │ • Access-Control-Allow-Origin: *               │   │
│  │ • Access-Control-Allow-Methods:                │   │
│  │   GET, POST, OPTIONS                           │   │
│  │ • Access-Control-Allow-Headers:                │   │
│  │   Content-Type, x-auth-token, X-Auth-Token     │   │
│  │                                                │   │
│  │ OPTIONS Preflight:                             │   │
│  │ • Handled explicitly                           │   │
│  │ • Returns 204 No Content                       │   │
│  └────────────────────────────────────────────────┘   │
│                                                         │
│  4. Client-Side Session Management                      │
│  ┌────────────────────────────────────────────────┐   │
│  │ • Token stored in localStorage                 │   │
│  │ • Persistent across page refreshes             │   │
│  │ • Cleared on logout                            │   │
│  │ • Cleared after 3x 401 errors                  │   │
│  │ • No cookies (stateless)                       │   │
│  └────────────────────────────────────────────────┘   │
│                                                         │
│  5. Error Handling & Security                           │
│  ┌────────────────────────────────────────────────┐   │
│  │ 401 Unauthorized:                              │   │
│  │ • Track consecutive errors                     │   │
│  │ • After 3 errors: Force logout                 │   │
│  │ • Clear token and local storage                │   │
│  │ • Redirect to login                            │   │
│  │                                                │   │
│  │ Invalid JSON:                                  │   │
│  │ • Return 400 Bad Request                       │   │
│  │ • Log error                                    │   │
│  │                                                │   │
│  │ Unknown Actions:                               │   │
│  │ • Return 400 with error message                │   │
│  └────────────────────────────────────────────────┘   │
│                                                         │
│  6. WiFi Network Security                               │
│  ┌────────────────────────────────────────────────┐   │
│  │ Access Point Configuration:                    │   │
│  │ • SSID: "group56bridge"                        │   │
│  │ • Password: "bridgehereweare"                  │   │
│  │ • WPA2 encryption                              │   │
│  │ • IP Range: 192.168.4.x                        │   │
│  │ • Gateway: 192.168.4.1 (ESP32)                 │   │
│  │                                                │   │
│  │ Network Isolation:                             │   │
│  │ • Dedicated WiFi network                       │   │
│  │ • No internet gateway                          │   │
│  │ • Local LAN only                               │   │
│  └────────────────────────────────────────────────┘   │
│                                                         │
│  7. Input Validation                                    │
│  ┌────────────────────────────────────────────────┐   │
│  │ • JSON parsing with error handling             │   │
│  │ • Action whitelist validation                  │   │
│  │ • State boundary checks                        │   │
│  │ • Limit switch safety checks                   │   │
│  └────────────────────────────────────────────────┘   │
│                                                         │
└─────────────────────────────────────────────────────────┘


security limitation and recommendations 
┌─────────────────────────────────────────────────────────┐
│         CURRENT LIMITATIONS & IMPROVEMENTS              │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  Current Limitations:                                   │
│  • Simple random token (not cryptographically secure)   │
│  • Hardcoded credentials (admin/admin)                  │
│  • Single user session only                             │
│  • No token expiration time                             │
│  • No HTTPS/TLS encryption                              │
│  • No rate limiting                                     │
│  • No audit logging                                     │
│                                                         │
│  Recommended Improvements:                              │
│  ┌────────────────────────────────────────────────┐   │
│  │ 1. Use cryptographic hash for tokens           │   │
│  │    • SHA-256 or similar                        │   │
│  │    • Longer token strings                ┌─────────────────────────────────────────────────────────┐
│         CURRENT LIMITATIONS & IMPROVEMENTS              │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  Current Limitations:                                   │
│  • Simple random token (not cryptographically secure)   │
│  • Hardcoded credentials (admin/admin)                  │
│  • Single user session only                             │
│  • No token expiration time                             │
│  • No HTTPS/TLS encryption                              │
│  • No rate limiting                                     │
│  • No audit logging                                     │
│                                                         │
│  Recommended Improvements:                              │
│  ┌────────────────────────────────────────────────┐   │
│  │ 1. Use cryptographic hash for tokens           │   │
│  │    • SHA-256 or similar                        │   │
│  │    • Longer token strings                 


now help me to focus on part 10, part 11 and part 12  
hardware integration and testing and validation focus on software 