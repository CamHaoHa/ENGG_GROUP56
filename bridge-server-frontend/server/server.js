const express = require('express');
const bodyParser = require('body-parser');
const app = express();
const port = 3000;

// Hardcoded credentials (use secure auth in production)
const validUsername = 'admin';
const validPassword = 'admin';

// State storage
let state = {
    currentState: 'Default State',
    bridgeState: false,
    redLedA: false,
    yellowLedA: false,
    greenLedA: true,
    redLedB: true,
    yellowLedB: false,
    greenLedB: false,
    commands: { open: false, close: false, clear: false }
};

// Middleware
app.use(bodyParser.json());
app.use(bodyParser.urlencoded({ extended: true }));
app.use(express.static('public'));

// Session management (simplified)
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
    .button.clear { background: #2196F3; color: #fff; }
    .button.logout { background: #888; color: #fff; }
    .bridge-container { width: 300px; height: 200px; margin: 20px auto; background: #e0f7fa; border: 1px solid #ccc; }
    .bridge { transition: transform 3s ease-in-out; }
    .bridge.open { transform: translateY(-50px); }
    .bridge.closed { transform: translateY(0); }
    .bridge.opening, .bridge.closing { animation: pulse 1s infinite; }
    @keyframes pulse { 0% { opacity: 1; } 50% { opacity: 0.5; } 100% { opacity: 1; } }
    .fallback { color: red; font-size: 1rem; margin-top: 10px; }
    .traffic-lights, .boat-lights { font-size: 1.2rem; margin: 10px; }
    .light-status { display: inline-block; width: 20px; height: 20px; border-radius: 50%; margin: 0 5px; }
    .red { background: #F44336; }
    .yellow { background: #FFCA28; }
    .green { background: #4CAF50; }
    .off { background: #ccc; }
  </style>
</head>
<body>
  <h2>ESP32 Bridge Control</h2>
  <button class="button logout" id="logoutButton">Logout</button>
  <div class="status">State: <span id="state">${state.currentState}</span></div>
  <div class="status">Bridge: <span id="bridgeState">${state.bridgeState ? 'Open' : 'Closed'}</span></div>
  <div class="traffic-lights">Traffic Lights: 
    <span class="light-status ${state.redLedA ? 'red' : 'off'}" id="trafficRed"></span>
    <span class="light-status ${state.yellowLedA ? 'yellow' : 'off'}" id="trafficYellow"></span>
    <span class="light-status ${state.greenLedA ? 'green' : 'off'}" id="trafficGreen"></span>
  </div>
  <div class="boat-lights">Boat Lights: 
    <span class="light-status ${state.redLedB ? 'red' : 'off'}" id="boatRed"></span>
    <span class="light-status ${state.yellowLedB ? 'yellow' : 'off'}" id="boatYellow"></span>
    <span class="light-status ${state.greenLedB ? 'green' : 'off'}" id="boatGreen"></span>
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
  <button class="button open" id="openButton">Open Bridge</button>
  <button class="button close" id="closeButton">Close Bridge</button>
  <button class="button clear" id="clearButton">Clear</button>
  <script src="/control.js"></script>
</body>
</html>
  `);
});

// API Routes
app.post('/api/state', (req, res) => {
    const { state: newState, bridgeState, redLedA, yellowLedA, greenLedA, redLedB, yellowLedB, greenLedB } = req.body;
    state.currentState = newState;
    state.bridgeState = bridgeState;
    state.redLedA = redLedA;
    state.yellowLedA = yellowLedA;
    state.greenLedA = greenLedA;
    state.redLedB = redLedB;
    state.yellowLedB = yellowLedB;
    state.greenLedB = greenLedB;
    console.log(`Received state update: ${JSON.stringify(state)}`);
    res.sendStatus(200);
});

app.get('/api/state', (req, res) => {
    console.log(`Sending state to client: ${JSON.stringify(state)}`);
    res.json(state);
});

app.get('/api/commands', (req, res) => {
    console.log(`Sending commands: ${JSON.stringify(state.commands)}`);
    const commands = state.commands;
    state.commands = { open: false, close: false, clear: false }; // Reset after sending
    res.json(commands);
});

app.post('/api/control', (req, res) => {
    const { action } = req.body;
    console.log(`Received control action: ${action}`);
    if (action === 'open') {
        state.commands.open = true;
    } else if (action === 'close') {
        state.commands.close = true;
    } else if (action === 'clear') {
        state.commands.clear = true;
    }
    res.sendStatus(200);
});

app.listen(port, () => {
    console.log(`Server running at http://localhost:${port}`);
});