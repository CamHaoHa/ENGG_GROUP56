const express = require("express");
const app = express();
const http = require("http").createServer(app);
const io = require("socket.io")(http, { cors: { origin: "*" } });
const bodyParser = require("body-parser");

// Hardcoded credentials (secure in production)
const validUsername = "admin";
const validPassword = "admin";

// State storage
let state = {
    currentState: 0,
    bridgeState: false,
    redLedA: false,
    yellowLedA: false,
    greenLedA: true,
    redLedB: true,
    yellowLedB: false,
    greenLedB: false,
    commands: { open: false, close: false, clear: false },
};

// Middleware
app.use(bodyParser.json());
app.use(bodyParser.urlencoded({ extended: true }));
app.use(express.static(__dirname + "/frontend/build"));
app.use((req, res, next) => {
    res.header("Access-Control-Allow-Origin", "*");
    res.header("Access-Control-Allow-Methods", "GET, POST");
    res.header("Access-Control-Allow-Headers", "Content-Type, x-auth");
    next();
});

// Routes
app.post("/api/login", (req, res) => {
    const { username, password } = req.body;
    console.log(`[${new Date().toISOString()}] API Login attempt:`, {
        username,
    });
    if (username === validUsername && password === validPassword) {
        console.log(`[${new Date().toISOString()}] API Login successful`);
        res.json({ success: true });
    } else {
        console.log(`[${new Date().toISOString()}] API Login failed`);
        res.status(401).json({
            success: false,
            message: "Invalid credentials",
        });
    }
});

app.get("/api/auth-status", (req, res) => {
    const isAuthenticated = req.headers["x-auth"] === "true";
    console.log(
        `[${new Date().toISOString()}] Auth-status requested, isAuthenticated: ${isAuthenticated}`
    );
    res.json({ isAuthenticated });
});

app.get("/", (req, res) => {
    res.sendFile(__dirname + "/frontend/build/index.html");
});

app.get("/login", (req, res) => {
    res.sendFile(__dirname + "/frontend/build/index.html");
});

app.get("/error", (req, res) => {
    res.status(401).send(
        'Unauthorized<br>Wrong credentials. <a href="/login">Try again</a>.'
    );
});

app.get("/logout", (req, res) => {
    res.send('Logged out. <a href="/login">Login again</a>');
});

io.on("connection", (socket) => {
    console.log(
        `[${new Date().toISOString()}] Client connected: ${socket.id}, IP: ${
            socket.handshake.address
        }, User-Agent: ${socket.handshake.headers["user-agent"] || "unknown"}`
    );
    socket.emit("update", state);

    socket.on("espUpdate", (data) => {
        console.log(`[${new Date().toISOString()}] ESP32 update:`, data);
        const validKeys = [
            "currentState",
            "bridgeState",
            "redLedA",
            "yellowLedA",
            "greenLedA",
            "redLedB",
            "yellowLedB",
            "greenLedB",
        ];
        const filteredData = Object.keys(data)
            .filter((key) => validKeys.includes(key))
            .reduce((obj, key) => ({ ...obj, [key]: data[key] }), {});
        if (Object.keys(filteredData).length > 0) {
            state = { ...state, ...filteredData };
            io.emit("update", state);
        } else {
            console.log(
                `[${new Date().toISOString()}] Invalid ESP32 data, ignoring`
            );
        }
    });

    socket.on("command", (cmd) => {
        console.log(`[${new Date().toISOString()}] Command received:`, cmd);
        state.commands = { ...state.commands, ...cmd };
        io.emit("command", state.commands);
        state.commands = { open: false, close: false, clear: false };
    });

    socket.on("disconnect", (reason) => {
        console.log(
            `[${new Date().toISOString()}] Client disconnected: ${
                socket.id
            }, Reason: ${reason}`
        );
    });
});

const PORT = process.env.PORT || 3000;
http.listen(PORT, () =>
    console.log(`[${new Date().toISOString()}] Server running on port ${PORT}`)
);
