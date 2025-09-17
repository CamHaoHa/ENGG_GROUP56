import React, { useState, useEffect } from "react";
import { Card, Typography, Button, Box } from "@mui/material";
import { motion } from "framer-motion";
import Login from "./Login";

const API_URL = "http://192.168.4.1"; // ESP32 AP IP (default for WiFi.softAP)
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
        greenLedA: false,
        redLedB: false,
        yellowLedB: false,
        greenLedB: false,
    });
    const [error, setError] = useState(null);
    const [isAuthenticated, setIsAuthenticated] = useState(
        localStorage.getItem("isAuthenticated") === "true"
    );
    const [logoutMessage, setLogoutMessage] = useState(null);

    useEffect(() => {
        if (!isAuthenticated) return;

        const fetchState = () => {
            fetch(`${API_URL}/api/state`, {
                headers: { "x-auth": "true" },
            })
                .then((res) => {
                    if (!res.ok) throw new Error("Failed to fetch state");
                    return res.json();
                })
                .then((newData) => {
                    setData(newData);
                    setError(null);
                })
                .catch((err) => {
                    console.error("State fetch error:", err);
                    setError("Failed to connect to ESP32");
                });
        };

        fetchState(); // Initial fetch
        const interval = setInterval(fetchState, 500); // Poll every 500ms
        return () => clearInterval(interval);
    }, [isAuthenticated]);

    const sendCommand = (action) => {
        fetch(`${API_URL}/api/command`, {
            method: "POST",
            headers: { "Content-Type": "application/json", "x-auth": "true" },
            body: JSON.stringify({ action }),
        })
            .then((res) => {
                if (!res.ok) throw new Error("Command failed");
                return res.json();
            })
            .catch((err) => {
                console.error("Command error:", err);
                setError("Failed to send command");
            });
    };

    const handleLogout = () => {
        fetch(`${API_URL}/api/logout`)
            .then(() => {
                localStorage.removeItem("isAuthenticated");
                setIsAuthenticated(false);
                setLogoutMessage(
                    'Logged out. <a href="/login">Login again</a>'
                );
            })
            .catch(() => setError("Failed to logout"));
    };

    if (logoutMessage) {
        return (
            <Box
                sx={{ p: 4, textAlign: "center" }}
                dangerouslySetInnerHTML={{ __html: logoutMessage }}
            />
        );
    }

    if (!isAuthenticated) {
        return (
            <Login
                setIsAuthenticated={setIsAuthenticated}
                setError={setError}
            />
        );
    }

    if (error) {
        return (
            <Typography color="error" sx={{ p: 4, textAlign: "center" }}>
                {error}
            </Typography>
        );
    }

    const isOpening = data.currentState === 3;
    const isClosing = data.currentState === 8;
    const bridgeLabel = isOpening
        ? "Opening"
        : isClosing
        ? "Closing"
        : data.bridgeState
        ? "Open"
        : "Closed";

    return (
        <Box
            sx={{
                p: 4,
                maxWidth: 600,
                mx: "auto",
                textAlign: "center",
                fontFamily: "Arial, sans-serif",
            }}
        >
            <Card sx={{ p: 3, background: "#f0f0f0" }}>
                <Typography
                    variant="h4"
                    gutterBottom
                    sx={{
                        fontSize: "1.8rem",
                        color: "#fff",
                        background: "#333",
                        p: 1,
                    }}
                >
                    ESP32 Bridge Control
                </Typography>
                <Typography
                    variant="h6"
                    sx={{ fontSize: "1.2rem", color: "#555" }}
                >
                    State: {stateNames[data.currentState] || "Unknown"}
                </Typography>
                <Typography sx={{ fontSize: "1.2rem", color: "#555" }}>
                    Bridge: {bridgeLabel}
                </Typography>

                <Box
                    sx={{
                        width: 300,
                        height: 200,
                        mx: "auto",
                        my: 2,
                        background: "#e0f7fa",
                        border: "1px solid #ccc",
                    }}
                >
                    <motion.div
                        className="bridge"
                        animate={{
                            y: data.bridgeState ? -50 : 0,
                            opacity: isOpening || isClosing ? [1, 0.5, 1] : 1,
                        }}
                        transition={{
                            y: { duration: 3, ease: "easeInOut" },
                            opacity: {
                                duration: 1,
                                repeat: isOpening || isClosing ? Infinity : 0,
                            },
                        }}
                    >
                        <Box
                            sx={{
                                width: "100%",
                                height: 50,
                                bgcolor: "gray",
                                borderRadius: 2,
                            }}
                        />
                    </motion.div>
                </Box>

                <Box sx={{ mt: 2 }}>
                    <Typography sx={{ fontSize: "1.2rem" }}>
                        Traffic Lights:
                    </Typography>
                    <Box
                        sx={{
                            display: "flex",
                            justifyContent: "center",
                            gap: 1,
                        }}
                    >
                        <Box
                            sx={{
                                width: 20,
                                height: 20,
                                borderRadius: "50%",
                                bgcolor: data.redLedA ? "#F44336" : "#ccc",
                            }}
                        />
                        <Box
                            sx={{
                                width: 20,
                                height: 20,
                                borderRadius: "50%",
                                bgcolor: data.yellowLedA ? "#FFCA28" : "#ccc",
                            }}
                        />
                        <Box
                            sx={{
                                width: 20,
                                height: 20,
                                borderRadius: "50%",
                                bgcolor: data.greenLedA ? "#4CAF50" : "#ccc",
                            }}
                        />
                    </Box>
                </Box>
                <Box sx={{ mt: 2 }}>
                    <Typography sx={{ fontSize: "1.2rem" }}>
                        Boat Lights:
                    </Typography>
                    <Box
                        sx={{
                            display: "flex",
                            justifyContent: "center",
                            gap: 1,
                        }}
                    >
                        <Box
                            sx={{
                                width: 20,
                                height: 20,
                                borderRadius: "50%",
                                bgcolor: data.redLedB ? "#F44336" : "#ccc",
                            }}
                        />
                        <Box
                            sx={{
                                width: 20,
                                height: 20,
                                borderRadius: "50%",
                                bgcolor: data.yellowLedB ? "#FFCA28" : "#ccc",
                            }}
                        />
                        <Box
                            sx={{
                                width: 20,
                                height: 20,
                                borderRadius: "50%",
                                bgcolor: data.greenLedB ? "#4CAF50" : "#ccc",
                            }}
                        />
                    </Box>
                </Box>

                <Box sx={{ mt: 3 }}>
                    <Button
                        variant="contained"
                        onClick={() => sendCommand("open")}
                        disabled={data.bridgeState}
                        sx={{
                            mr: 2,
                            px: 3,
                            py: 1.5,
                            background: "#4CAF50",
                            color: "#fff",
                        }}
                    >
                        Open Bridge
                    </Button>
                    <Button
                        variant="contained"
                        onClick={() => sendCommand("close")}
                        disabled={!data.bridgeState}
                        sx={{
                            mr: 2,
                            px: 3,
                            py: 1.5,
                            background: "#F44336",
                            color: "#fff",
                        }}
                    >
                        Close Bridge
                    </Button>
                    <Button
                        variant="contained"
                        onClick={() => sendCommand("clear")}
                        sx={{
                            px: 3,
                            py: 1.5,
                            background: "#2196F3",
                            color: "#fff",
                        }}
                    >
                        Clear
                    </Button>
                </Box>
                <Button
                    variant="outlined"
                    onClick={handleLogout}
                    sx={{
                        mt: 2,
                        px: 3,
                        py: 1.5,
                        background: "#888",
                        color: "#fff",
                    }}
                >
                    Logout
                </Button>
            </Card>
        </Box>
    );
}

export default App;
