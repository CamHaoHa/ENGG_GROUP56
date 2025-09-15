const express = require("express");
const app = express();
const http = require("http").createServer(app);
const io = require("socket.io")(http, {
    cors: { origin: "*" }, // Allow frontend connections; secure later
});

let currentState = { state: 0, distance: 0 }; // Placeholder

io.on("connection", (socket) => {
    console.log("Client connected:", socket.id);
    socket.emit("update", currentState); // Sync on connect

    socket.on("espUpdate", (data) => {
        console.log("Update from ESP32:", data);
        currentState = data;
        io.emit("update", currentState); // Broadcast to frontends
    });

    socket.on("override", (cmd) => {
        console.log("Override from UI:", cmd);
        // Validate and send to ESP32
        io.emit("command", cmd); // ESP32 listens for this
    });

    socket.on("disconnect", () => console.log("Client disconnected"));
});

const PORT = 3000;
http.listen(PORT, () => console.log(`Server running on port ${PORT}`));
