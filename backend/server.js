const express = require("express");
const http = require("http");
const WebSocket = require("ws");

const app = express();
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

let latestData = {
    // Initial state
    state: "System Ready",
    shipDetected: false,
    liftHeight: 0,
    motorCurrent: 0,
    trafficSignal: "Green",
    boatSignal: "Off",
};

app.use(express.json());
app.use(express.static("public")); // Serve UI files from 'public' folder

// Receive JSON from ESP32
app.post("/update", (req, res) => {
    latestData = req.body;
    console.log("Received data:", latestData);
    broadcastData(); // Push to connected clients
    res.sendStatus(200);
});

// WebSocket for live updates
wss.on("connection", (ws) => {
    console.log("Client connected");
    ws.send(JSON.stringify(latestData)); // Send initial data
    ws.on("close", () => console.log("Client disconnected"));
});

function broadcastData() {
    wss.clients.forEach((client) => {
        if (client.readyState === WebSocket.OPEN) {
            client.send(JSON.stringify(latestData));
        }
    });
}

server.listen(3000, () =>
    console.log("Server running on http://localhost:3000")
);
