const express = require('express');
const bodyParser = require('body-parser');
const app = express();
const port = 3000;

// Hardcoded credentials (use secure auth in production, e.g., JWT)
const validUsername = 'admin';
const validPassword = 'admin';

// State storage (use a database like MongoDB in production)
let state = {
    bridgeState: false, // false = closed, true = open
    shipDetected: false,
    redLed: false,
    yellowLed: false,
    greenLed: false,
    bridgeAction: 'none', // 'none', 'open', 'close'
    bridgeActionTimestamp: 0,
    clearShip: false, // Flag for clear ship command
};

// Middleware
app.use(bodyParser.json());
app.use(bodyParser.urlencoded({ extended: true }));
app.use(express.static('public')); // Serve static files (e.g., control.js)

// Session management (simplified, use express-session in production)
let isAuthenticated = false;

// Routes: Web Interface
app.get('/', (req, res) => {
    if (!isAuthenticated) {
        res.send(`
<!DOCTYPE HTML>
<html>
<head>
  <title>ESP32 Login</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html { font-family: Arial, sans-serif; text-align: center; }
    h1 { font-size: 1.8rem; color: #fff; background: #333; padding: 10px; }
    .form { margin: 20px auto; width: 200px; }
    input { margin: 5px; padding: 8px; width: 100%; font-size: 1rem; }
    .button { width: 100%; padding: 10px; background: #4CAF50; color: #fff; border: none; border-radius: 5px; cursor: pointer; }
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
</html>
    `);
    } else {
        res.redirect('/control');
    }
});

app.post('/login', (req, res) => {
    const { username, password } = req.body;
    if (username === validUsername && password === validPassword) {
        isAuthenticated = true;
        res.redirect('/control');
    } else {
        res.status(401).send('<h1>Unauthorized</h1><p>Wrong credentials. <a href="/">Try again</a>.</p>');
    }
});

app.get('/logout', (req, res) => {
    isAuthenticated = false;
    res.send(`
<!DOCTYPE HTML>
<html>
<head>
  <title>ESP32 Logout</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html { font-family: Arial, sans-serif; text-align: center; }
    p { font-size: 1.2rem; color: #555; }
  </style>
</head>
<body>
  <p>Logged out. <a href="/">Login</a>.</p>
</body>
</html>
  `);
});

app.get('/control', (req, res) => {
    if (!isAuthenticated) {
        res.redirect('/');
        return;
    }
    res.send(`
<!DOCTYPE HTML>
<html>
<head>
  <title>ESP32 Bridge</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html { font-family: Arial, sans-serif; text-align: center; }
    body { max-width: 600px; margin: 0 auto; padding: 10px; background: #f0f0f0; }
    h2 { font-size: 2rem; color: #333; }
    .status { font-size: 1.2rem; margin: 20px; color: #555; padding: 5px; }
    .button { padding: 12px 24px; margin: 10px; font-size: 1rem; border: none; border-radius: 5px; cursor: pointer; }
    .button.open { background: #4CAF50; color: #fff; }
    .button.close { background: #F44336; color: #fff; }
    .button.logout { background: #888; color: #fff; }
    .button.red { background: #F44336; color: #fff; }
    .button.yellow { background: #FFCA28; color: #000; }
    .button.green { background: #4CAF50; color: #fff; }
    .button.clear { background: #2196F3; color: #fff; }
    .bridge-container { width: 300px; height: 200px; margin: 20px auto; background: #e0f7fa; border: 1px solid #ccc; }
    .bridge { transition: transform 3s ease-in-out; }
    .bridge.open { transform: translateY(-50px); }
    .bridge.closed { transform: translateY(0); }
    .bridge.opening { animation: pulse 1s infinite; }
    .bridge.closing { animation: pulse 1s infinite; }
    @keyframes pulse { 0% { opacity: 1; } 50% { opacity: 0.5; } 100% { opacity: 1; } }
    .fallback { color: red; font-size: 1rem; margin-top: 10px; }
    .ship-detected.flashing { animation: flash 0.5s infinite; border: 2px solid #FFCA28; border-radius: 5px; padding: 8px; background: #333; }
    @keyframes flash { 0% { opacity: 1; color: #FFCA28; } 50% { opacity: 0.7; color: #F44336; } 100% { opacity: 1; color: #FFCA28; } }
  </style>
</head>
<body>
  <h2>ESP32 Bridge Control</h2>
  <button class="button logout" id="logoutButton">Logout</button>
  <div class="status">Bridge: <span id="state">${state.bridgeState ? 'Open' : 'Closed'}</span></div>
  <div class="status ship-detected ${state.shipDetected ? 'flashing' : ''}" id="shipStatus">
    <span id="shipState">${state.shipDetected ? 'Detected' : 'None'}</span>
  </div>
  <div class="bridge-container">
    <svg width="300" height="200" viewBox="0 0 300 200">
      <path d="M0 180 C50 170,100 190,150 180 C200 170,250 190,300 180 L300 200 L0 200 Z" fill="#4fc3f7"/>
      <rect x="50" y="50" width="20" height="130" fill="#666"/>
      <rect x="230" y="50" width="20" height="130" fill="#666"/>
      <rect x="70" y="100" width="160" height="20" fill="#888" id="bridge" class="bridge ${state.bridgeState ? 'open' : 'closed'}"/>
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
    <button class="button red" id="redLedButton" data-state="${state.redLed ? '1' : '0'}">Red LED ${state.redLed ? 'Off' : 'On'}</button>
    <button class="button yellow" id="yellowLedButton" data-state="${state.yellowLed ? '1' : '0'}">Yellow LED ${state.yellowLed ? 'Off' : 'On'}</button>
    <button class="button green" id="greenLedButton" data-state="${state.greenLed ? '1' : '0'}">Green LED ${state.greenLed ? 'Off' : 'On'}</button>
  </div>
  <script src="/control.js"></script>
</body>
</html>
  `);
});

// API Routes: ESP32 Communication
app.post('/api/state', (req, res) => {
    const { bridgeState, shipDetected, redLed, yellowLed, greenLed } = req.body;
    state.bridgeState = bridgeState;
    state.shipDetected = shipDetected;
    state.redLed = redLed;
    state.yellowLed = yellowLed;
    state.greenLed = greenLed;
    res.sendStatus(200);
});

app.post('/api/ship-state', (req, res) => {
    const { shipDetected } = req.body;
    state.shipDetected = shipDetected;
    if (shipDetected) {
        state.bridgeAction = 'open';
        state.bridgeActionTimestamp = Date.now();
    } else {
        state.bridgeAction = 'close';
        state.bridgeActionTimestamp = Date.now();
    }
    res.sendStatus(200);
});

app.get('/api/commands', (req, res) => {
    // Return commands, reset clearShip flag after sending
    const commands = {
        bridge: state.bridgeAction === 'open' ? true : state.bridgeAction === 'close' ? false : state.bridgeState,
        redLed: state.redLed,
        yellowLed: state.yellowLed,
        greenLed: state.greenLed,
        clearShip: state.clearShip,
    };
    state.clearShip = false; // Reset after sending
    res.json(commands);
});

app.post('/api/control-bridge', (req, res) => {
    const { action } = req.body;
    if (action === 'open' || action === 'close') {
        state.bridgeAction = action;
        state.bridgeActionTimestamp = Date.now();
        res.sendStatus(200);
    } else {
        res.sendStatus(400);
    }
});

app.post('/api/control-led', (req, res) => {
    const { led, state: ledState } = req.body;
    if (['redLed', 'yellowLed', 'greenLed'].includes(led)) {
        state[led] = ledState === '1';
        res.sendStatus(200);
    } else {
        res.sendStatus(400);
    }
});

app.get('/api/clear-ship', (req, res) => {
    state.clearShip = true;
    state.shipDetected = false;
    state.bridgeAction = 'close';
    state.bridgeActionTimestamp = Date.now();
    res.sendStatus(200);
});

app.listen(port, () => {
    console.log(`Server running at http://localhost:${port}`);
});