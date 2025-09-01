const express = require('express');
const bodyParser = require('body-parser');
const path = require('path');
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
app.use(express.static(path.join(__dirname, 'public')));

// Session management (simplified)
let isAuthenticated = false;

// Routes: Web Interface
app.get('/', (req, res) => {
    if (!isAuthenticated) {
        res.sendFile(path.join(__dirname, 'public', 'login.html'));
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
        res.status(401).sendFile(path.join(__dirname, 'public', 'error.html'));
    }
});

app.get('/logout', (req, res) => {
    isAuthenticated = false;
    res.sendFile(path.join(__dirname, 'public', 'logout.html'));
});

app.get('/control', (req, res) => {
    if (!isAuthenticated) {
        res.redirect('/');
    } else {
        res.sendFile(path.join(__dirname, 'public', 'control.html'));
    }
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