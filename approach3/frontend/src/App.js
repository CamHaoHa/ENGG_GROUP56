import React, { useState, useEffect } from "react";
import { Card, Typography, Button, Box } from "@mui/material";
import { motion } from "framer-motion";
import io from "socket.io-client";

const SOCKET_SERVER = "http://192.168.0.19:3000"; // Update to server IP

const stateNames = [
    "Default (Bridge Closed)",
    "Ship Detected",
    "Traffic Clear",
    "Pending Open",
    "Bridge Open",
    "Boat Passing",
    "Stopping Boat",
    "Pending Close",
    "Bridge Closing",
    "Traffic Ready",
];

function App() {
    const [data, setData] = useState({
        currentState: 0,
        bridgeState: false,
        redLedA: false,
        yellowLedA: false,
        greenLedA: true,
        redLedB: true,
        yellowLedB: false,
        greenLedB: false,
    });
    const socket = io(SOCKET_SERVER, { autoConnect: false });

    useEffect(() => {
        socket.connect();
        socket.on("update", (newData) => {
            setData(newData);
        });
        return () => {
            socket.disconnect();
        };
    }, [socket]);

    const sendCommand = (action) => {
        socket.emit("command", { [action]: true });
    };

    return (
        <Box sx={{ p: 4, maxWidth: 800, mx: "auto" }}>
            <Card sx={{ p: 3 }}>
                <Typography variant="h4" gutterBottom>
                    Bridge Control Dashboard
                </Typography>
                <Typography variant="h6">
                    State: {stateNames[data.currentState]}
                </Typography>
                <Typography>
                    Bridge: {data.bridgeState ? "Open" : "Closed"}
                </Typography>

                {/* Bridge Animation */}
                <motion.div
                    animate={{ y: data.bridgeState ? -100 : 0 }}
                    transition={{ duration: 1 }}
                    style={{ margin: "20px 0" }}
                >
                    <Box
                        sx={{
                            width: 200,
                            height: 50,
                            bgcolor: "gray",
                            borderRadius: 2,
                        }}
                    >
                        <Typography color="white" align="center" pt={2}>
                            Bridge
                        </Typography>
                    </Box>
                </motion.div>

                {/* Traffic and Boat Lights */}
                <Box
                    sx={{
                        display: "flex",
                        justifyContent: "space-between",
                        mt: 2,
                    }}
                >
                    <Box>
                        <Typography>Traffic (Side A)</Typography>
                        <Box sx={{ display: "flex", gap: 1 }}>
                            <Box
                                sx={{
                                    width: 20,
                                    height: 20,
                                    borderRadius: "50%",
                                    bgcolor: data.redLedA ? "red" : "gray",
                                }}
                            />
                            <Box
                                sx={{
                                    width: 20,
                                    height: 20,
                                    borderRadius: "50%",
                                    bgcolor: data.yellowLedA
                                        ? "yellow"
                                        : "gray",
                                }}
                            />
                            <Box
                                sx={{
                                    width: 20,
                                    height: 20,
                                    borderRadius: "50%",
                                    bgcolor: data.greenLedA ? "green" : "gray",
                                }}
                            />
                        </Box>
                    </Box>
                    <Box>
                        <Typography>Boat (Side B)</Typography>
                        <Box sx={{ display: "flex", gap: 1 }}>
                            <Box
                                sx={{
                                    width: 20,
                                    height: 20,
                                    borderRadius: "50%",
                                    bgcolor: data.redLedB ? "red" : "gray",
                                }}
                            />
                            <Box
                                sx={{
                                    width: 20,
                                    height: 20,
                                    borderRadius: "50%",
                                    bgcolor: data.yellowLedB
                                        ? "yellow"
                                        : "gray",
                                }}
                            />
                            <Box
                                sx={{
                                    width: 20,
                                    height: 20,
                                    borderRadius: "50%",
                                    bgcolor: data.greenLedB ? "green" : "gray",
                                }}
                            />
                        </Box>
                    </Box>
                </Box>

                {/* Controls */}
                <Box sx={{ mt: 3 }}>
                    <Button
                        variant="contained"
                        onClick={() => sendCommand("open")}
                        sx={{ mr: 2 }}
                    >
                        Open Bridge
                    </Button>
                    <Button
                        variant="contained"
                        onClick={() => sendCommand("close")}
                        sx={{ mr: 2 }}
                    >
                        Close Bridge
                    </Button>
                    <Button
                        variant="contained"
                        onClick={() => sendCommand("clear")}
                    >
                        Clear Ship
                    </Button>
                    <Button
                        variant="outlined"
                        onClick={() => (window.location = "/logout")}
                        sx={{ ml: 2 }}
                    >
                        Logout
                    </Button>
                </Box>
            </Card>
        </Box>
    );
}

export default App;
