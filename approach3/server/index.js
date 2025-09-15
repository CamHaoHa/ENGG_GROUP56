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
    currentState: 0, // Numeric state for FSM
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

// Session (simplified)
let isAuthenticated = false;

// Routes
app.get("/login", (req, res) => {
    res.send(`
    <form method="POST" action="/login">
      <input type="text" name="username" placeholder="Username" required />
      <input type="password" name="password" placeholder="Password" required />
      <button type="submit">Login</button>
    </form>
  `);
});

app.post("/login", (req, res) => {
    const { username, password } = req.body;
    if (username === validUsername && password === validPassword) {
        isAuthenticated = true;
        res.redirect("/");
    } else {
        res.status(401).send("Invalid credentials");
    }
});

app.get("/logout", (req, res) => {
    isAuthenticated = false;
    res.send('Logged out. <a href="/login">Login again</a>');
});

app.get("/", (req, res) => {
    if (!isAuthenticated) {
        res.redirect("/login");
    } else {
        res.sendFile(__dirname + "/frontend/build/index.html");
    }
});

io.on("connection", (socket) => {
    console.log(`Client connected: ${socket.id}`);
    if (isAuthenticated) {
        socket.emit("update", state); // Send current state
    }

    socket.on("espUpdate", (data) => {
        console.log("ESP32 update:", data);
        state = { ...state, ...data };
        io.emit("update", state); // Broadcast to clients
    });

    socket.on("command", (cmd) => {
        if (isAuthenticated) {
            console.log("Command received:", cmd);
            state.commands = { ...state.commands, ...cmd };
            io.emit("command", state.commands); // Forward to ESP32
            state.commands = { open: false, close: false, clear: false }; // Reset
        }
    });

    socket.on("disconnect", () =>
        console.log(`Client disconnected: ${socket.id}`)
    );
});

const PORT = 3000;
http.listen(PORT, () => console.log(`Server running on port ${PORT}`));
