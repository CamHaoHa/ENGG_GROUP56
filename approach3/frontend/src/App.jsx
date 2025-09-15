import React, { useState, useEffect } from "react";
import { Card, Typography, Button, Box } from "@mui/material";
import { motion } from "framer-motion";
import io from "socket.io-client";
import Login from "./Login";

const SOCKET_SERVER =
    process.env.REACT_APP_SOCKET_SERVER || "http://192.168.0.19:3000";
const socket = io(SOCKET_SERVER, {
    autoConnect: false,
    reconnectionAttempts: 3,
    reconnectionDelay: 1000,
});

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
    const [error, setError] = useState(null);
    const [isAuthenticated, setIsAuthenticated] = useState(
        localStorage.getItem("isAuthenticated") === "true"
    );
    const [logoutMessage, setLogoutMessage] = useState(null);

    useEffect(() => {
        socket.on("connect", () => {
            console.log(`Socket connected: ${socket.id}`);
        });
        socket.on("disconnect", (reason) => {
            console.log(`Socket disconnected: ${reason}`);
        });
        socket.on("update", (newData) => {
            console.log("Received state update:", newData);
            setData(newData);
            setError(null);
        });
        socket.on("connect_error", (err) => {
            console.error("Socket connect error:", err.message);
            setError("Failed to connect to server");
        });

        if (isAuthenticated) {
            console.log(
                "Attempting socket connection due to isAuthenticated=true"
            );
            socket.connect();
        } else {
            console.log("Checking auth status");
            fetch(`${SOCKET_SERVER}/api/auth-status`, {
                headers: {
                    "x-auth":
                        localStorage.getItem("isAuthenticated") === "true"
                            ? "true"
                            : "false",
                },
            })
                .then((res) => {
                    console.log("Auth status response:", res.status);
                    return res.json();
                })
                .then((data) => {
                    console.log("Auth status data:", data);
                    if (data.isAuthenticated) {
                        localStorage.setItem("isAuthenticated", "true");
                        setIsAuthenticated(true);
                        socket.connect();
                    } else {
                        setIsAuthenticated(false);
                    }
                })
                .catch((err) => {
                    console.error("Auth status check error:", err);
                    setError("Failed to check authentication");
                });
        }

        return () => {
            socket.off("connect");
            socket.off("disconnect");
            socket.off("update");
            socket.off("connect_error");
            socket.disconnect();
        };
    }, []);

    const sendCommand = (action) => {
        console.log("Sending command:", action);
        socket.emit("command", { [action]: true });
    };

    const handleLogout = () => {
        fetch(`${SOCKET_SERVER}/logout`)
            .then(() => {
                localStorage.removeItem("isAuthenticated");
                setIsAuthenticated(false);
                socket.disconnect();
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
