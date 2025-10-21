import React, { useState, useEffect, memo } from "react";
import { Card, Typography, Button, Box, CircularProgress, Alert } from "@mui/material";
import { motion } from "framer-motion";
import Login from "./Login";
import TrafficIcon from "@mui/icons-material/Traffic";
import DirectionsBoatIcon from "@mui/icons-material/DirectionsBoat";
import LockOpenIcon from "@mui/icons-material/LockOpen";
import LockIcon from "@mui/icons-material/Lock";
import RefreshIcon from "@mui/icons-material/Refresh";
import LogoutIcon from "@mui/icons-material/Logout";
import BuildIcon from "@mui/icons-material/Build";
import AutoModeIcon from "@mui/icons-material/AutoMode";

const API_URL = "http://192.168.4.1";
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

const TrafficLight = memo(
    ({ title, icon, red, yellow, green }) => (
        <Box sx={{ textAlign: "center" }}>
            <Typography sx={{ fontSize: "1.2rem", color: "#444", mb: 1 }}>
                {icon} {title}
            </Typography>
            <Box
                sx={{
                    display: "flex",
                    flexDirection: "column",
                    alignItems: "center",
                    bgcolor: "#333",
                    p: 1,
                    borderRadius: 1,
                    gap: 0.5,
                    width: 60,
                }}
            >
                <Box
                    sx={{
                        width: 30,
                        height: 30,
                        borderRadius: "50%",
                        bgcolor: red ? "#F44336" : "#666",
                        border: red ? "3px solid #B71C1C" : "3px solid #444",
                    }}
                />
                <Box
                    sx={{
                        width: 30,
                        height: 30,
                        borderRadius: "50%",
                        bgcolor: yellow ? "#FFCA28" : "#666",
                        border: yellow ? "3px solid #F57F17" : "3px solid #444",
                    }}
                />
                <Box
                    sx={{
                        width: 30,
                        height: 30,
                        borderRadius: "50%",
                        bgcolor: green ? "#4CAF50" : "#666",
                        border: green ? "3px solid #2E7D32" : "3px solid #444",
                    }}
                />
            </Box>
        </Box>
    ),
    (prevProps, nextProps) =>
        prevProps.red === nextProps.red &&
        prevProps.yellow === nextProps.yellow &&
        prevProps.green === nextProps.green
);

const BoatDetected = memo(
    ({ isPassing, isDetected }) => (
        <Box
            component={motion.div}
            animate={{
                backgroundColor: isPassing
                    ? ["#FFCA28", "#F57F17", "#FFCA28"]
                    : isDetected
                    ? "#FFCA28"
                    : "#E0E0E0",
                opacity: isPassing ? [1, 0.6, 1] : 1,
            }}
            transition={
                isPassing
                    ? { duration: 0.6, repeat: Infinity, ease: "easeInOut" }
                    : {}
            }
            sx={{
                p: 2,
                borderRadius: 2,
                textAlign: "center",
                fontSize: "1.2rem",
                fontWeight: 600,
                color: isPassing || isDetected ? "#000" : "#555",
                mb: 2,
            }}
        >
            Boat: {isPassing ? "Passing" : isDetected ? "Detected" : "None"}
        </Box>
    ),
    (prevProps, nextProps) =>
        prevProps.isPassing === nextProps.isPassing &&
        prevProps.isDetected === nextProps.isDetected
);

function App() {
    const [authToken, setAuthToken] = useState(
        localStorage.getItem("authToken") || ""
    );
    const [data, setData] = useState({
        currentState: 0,
        bridgeState: false,
        manualOverride: false,
        redLedA: false,
        yellowLedA: false,
        greenLedA: false,
        redLedB: false,
        yellowLedB: false,
        greenLedB: false,
    });
    const [error, setError] = useState(null);
    const [isAuthenticated, setIsAuthenticated] = useState(
        !!localStorage.getItem("authToken")
    );
    const [logoutMessage, setLogoutMessage] = useState(null);
    const [loading, setLoading] = useState(false);
    const [authErrorCount, setAuthErrorCount] = useState(0);

    const MAX_AUTH_ERRORS = 3;

    useEffect(() => {
        if (authToken) {
            localStorage.setItem("authToken", authToken);
        } else {
            localStorage.removeItem("authToken");
        }
    }, [authToken]);

    useEffect(() => {
        if (!isAuthenticated) return;

        const fetchState = async () => {
            const token = authToken;
            if (!token) {
                setIsAuthenticated(false);
                setError("No authentication token. Please log in.");
                return;
            }

            try {
                const url = `${API_URL}/api/state?token=${encodeURIComponent(token)}`;
                const res = await fetch(url, {
                    headers: { "x-auth-token": token },
                });

                if (res.status === 401) {
                    setAuthErrorCount((prev) => {
                        const newCount = prev + 1;
                        if (newCount >= MAX_AUTH_ERRORS) {
                            localStorage.removeItem("authToken");
                            setAuthToken("");
                            setIsAuthenticated(false);
                            setError("Session expired. Please log in again.");
                            return 0;
                        }
                        return newCount;
                    });
                    return;
                }
                if (!res.ok) throw new Error("Failed to fetch state");
                
                const newData = await res.json();
                console.log("=== FETCH STATE ===");
                console.log("Received data:", newData);
                console.log("Manual override status:", newData.manualOverride);
                console.log("Bridge state:", newData.bridgeState);
                console.log("Current state:", newData.currentState);
                
                setData((prev) => {
                    if (JSON.stringify(prev) === JSON.stringify(newData)) {
                        console.log("Data unchanged, skipping update");
                        return prev;
                    }
                    console.log("Data changed, updating state");
                    console.log("Previous data:", prev);
                    console.log("New data:", newData);
                    return newData;
                });
                
                setError(null);
                setAuthErrorCount(0);
            } catch (err) {
                setError(
                    "Cannot connect to ESP32. Ensure you're on the 'ESP32_Bridge' WiFi and the device is powered on."
                );
            } finally {
                setLoading(false);
            }
        };

        fetchState();
        const interval = setInterval(fetchState, 2000);
        return () => clearInterval(interval);
    }, [isAuthenticated, authToken]);

    const sendCommand = async (action) => {
        const token = authToken;
        console.log("=== SEND COMMAND ===");
        console.log("Action:", action);
        console.log("Token:", token);
        console.log("Current data state:", data);
        
        if (!token) {
            console.error("No token available!");
            setIsAuthenticated(false);
            setError("No authentication token. Please log in.");
            return;
        }
        setLoading(true);
        try {
            const url = `${API_URL}/api/command?token=${encodeURIComponent(token)}`;
            console.log("Sending POST to:", url);
            console.log("Request body:", JSON.stringify({ action }));
            
            const res = await fetch(url, {
                method: "POST",
                headers: {
                    "Content-Type": "application/json",
                    "x-auth-token": token,
                },
                body: JSON.stringify({ action }),
            });
            
            console.log("Response status:", res.status);
            console.log("Response ok:", res.ok);
            
            if (res.status === 401) {
                console.error("Unauthorized response!");
                setAuthErrorCount((prev) => {
                    const newCount = prev + 1;
                    if (newCount >= MAX_AUTH_ERRORS) {
                        console.error("Max auth errors reached, logging out");
                        localStorage.removeItem("authToken");
                        setAuthToken("");
                        setIsAuthenticated(false);
                        setError("Session expired. Please log in again.");
                        return 0;
                    }
                    return newCount;
                });
                return;
            }
            
            // Try to read response body
            const responseText = await res.text();
            console.log("Response body:", responseText);
            
            if (!res.ok) {
                console.error("Command failed with status:", res.status);
                throw new Error("Command failed");
            }
            
            console.log("Command successful!");
            setAuthErrorCount(0);
        } catch (err) {
            console.error("Send command error:", err);
            console.error("Error details:", err.message, err.stack);
            setError("Failed to send command. Check ESP32 connection.");
        } finally {
            setLoading(false);
            console.log("Command complete, loading set to false");
        }
    };

    const toggleOverride = async () => {
        const action = data.manualOverride ? "disableOverride" : "enableOverride";
        console.log("=== TOGGLE OVERRIDE ===");
        console.log("Current override state:", data.manualOverride);
        console.log("Sending action:", action);
        console.log("Auth token:", authToken);
        await sendCommand(action);
        console.log("Command sent, waiting for state update...");
    };

    const handleLogout = async () => {
        const token = authToken;
        try {
            const url = `${API_URL}/api/logout?token=${encodeURIComponent(token)}`;
            await fetch(url, {
                headers: { "x-auth-token": token },
            });
            localStorage.removeItem("authToken");
            setAuthToken("");
            setIsAuthenticated(false);
            setLogoutMessage(
                'Logged out successfully. <a href="/">Login again</a>'
            );
        } catch (err) {
            setError("Failed to logout. Try manually clearing storage.");
        }
    };

    if (logoutMessage) {
        return (
            <Box
                sx={{ p: 4, textAlign: "center", color: "#555" }}
                dangerouslySetInnerHTML={{ __html: logoutMessage }}
            />
        );
    }

    if (!isAuthenticated) {
        return (
            <Login
                setIsAuthenticated={setIsAuthenticated}
                setError={setError}
                setAuthToken={setAuthToken}
            />
        );
    }

    if (error) {
        return (
            <Typography
                color="error"
                sx={{ p: 4, textAlign: "center", fontSize: "1.2rem" }}
            >
                {error}
            </Typography>
        );
    }

    const isTransitioning = data.currentState === 3 || data.currentState === 8;
    const bridgeLabel = isTransitioning
        ? data.currentState === 3
            ? "Opening..."
            : "Closing..."
        : data.bridgeState
        ? "Open"
        : "Closed";

    const isBoatVisible = data.currentState === 1 || data.currentState === 5;

    return (
        <Box
            sx={{
                p: { xs: 2, md: 4 },
                maxWidth: 600,
                mx: "auto",
                textAlign: "center",
                fontFamily: "Arial, sans-serif",
                bgcolor: "#f5f5f5",
                borderRadius: 2,
                boxShadow: 3,
            }}
        >
            <Card
                sx={{
                    p: 3,
                    bgcolor: "#ffffff",
                    borderRadius: 2,
                    position: "relative",
                }}
            >
                <Typography
                    variant="h4"
                    gutterBottom
                    sx={{
                        fontSize: "1.8rem",
                        color: "#fff",
                        bgcolor: "#333",
                        p: 1.5,
                        borderRadius: 1,
                    }}
                >
                    ESP32 Bridge Control Panel
                </Typography>

                {/* Override Mode Alert */}
                {data.manualOverride && (
                    <Alert 
                        severity="warning" 
                        sx={{ mb: 2, fontWeight: 600 }}
                        icon={<BuildIcon />}
                    >
                        MANUAL OVERRIDE ACTIVE - Automatic operation disabled
                    </Alert>
                )}

                <Typography
                    variant="h6"
                    sx={{ fontSize: "1.4rem", color: "#444", mb: 1 }}
                >
                    Current State: {stateNames[data.currentState] || "Unknown"}
                </Typography>
                <Typography sx={{ fontSize: "1.2rem", color: "#666", mb: 1 }}>
                    Bridge Status: {bridgeLabel}
                </Typography>
                <Typography sx={{ fontSize: "1rem", color: "#888", mb: 2 }}>
                    Mode: {data.manualOverride ? "Manual Override" : "Automatic"}
                </Typography>

                {loading && <CircularProgress size={24} sx={{ mb: 2 }} />}

                {/* Bridge Animation */}
                <Box
                    sx={{
                        width: "100%",
                        height: 260,
                        mx: "auto",
                        my: 2,
                        bgcolor: "#b3e5fc",
                        border: "2px solid #0d47a1",
                        borderRadius: 4,
                        position: "relative",
                        overflow: "hidden",
                        boxShadow: "0 12px 24px rgba(0,0,0,0.3)",
                    }}
                >
                    <svg
                        width="100%"
                        height="100%"
                        viewBox="0 0 300 260"
                        preserveAspectRatio="xMidYMid meet"
                    >
                        <defs>
                            <linearGradient
                                id="skyGradient"
                                x1="0"
                                y1="0"
                                x2="0"
                                y2="1"
                            >
                                <stop offset="0%" stopColor="#0277bd" />
                                <stop offset="100%" stopColor="#b3e5fc" />
                            </linearGradient>
                            <linearGradient
                                id="bridgeGradient"
                                x1="0"
                                y1="0"
                                x2="1"
                                y2="0"
                            >
                                <stop offset="0%" stopColor="#455a64" />
                                <stop offset="100%" stopColor="#78909c" />
                            </linearGradient>
                            <filter
                                id="shadow"
                                x="-20%"
                                y="-20%"
                                width="140%"
                                height="140%"
                            >
                                <feDropShadow
                                    dx="4"
                                    dy="4"
                                    stdDeviation="6"
                                    floodColor="#000000"
                                    floodOpacity="0.4"
                                />
                            </filter>
                        </defs>
                        <rect
                            x="0"
                            y="0"
                            width="300"
                            height="170"
                            fill="url(#skyGradient)"
                        />

                        {/* Animated Water */}
                        <motion.g
                            animate={{ y: [0, -7, 0] }}
                            transition={{
                                repeat: Infinity,
                                duration: 4,
                                ease: "easeInOut",
                            }}
                        >
                            <path
                                d="M0 210 C30 200, 60 220, 90 210 C120 200, 150 220, 180 210 C210 200, 240 220, 270 210 C300 200, 330 220, 360 210 L360 260 L0 260 Z"
                                fill="#01579b"
                                opacity="0.9"
                            />
                            <path
                                d="M0 220 C25 210, 55 230, 85 220 C115 210, 145 230, 175 220 C205 210, 235 230, 265 220 C295 210, 325 230, 360 220 L360 260 L0 260 Z"
                                fill="#4fc3f7"
                                opacity="0.7"
                            />
                            <path
                                d="M0 215 C35 205, 65 225, 95 215 C125 205, 155 225, 185 215 C215 205, 245 225, 275 215 C305 205, 335 225, 360 215 L360 260 L0 260 Z"
                                fill="#81d4fa"
                                opacity="0.5"
                            />
                        </motion.g>

                        {/* Boat */}
                        {isBoatVisible && (
                            <motion.path
                                d="M100 210 L160 210 L170 230 L90 230 Z M110 210 L140 190 L150 210 Z"
                                fill="#212121"
                                animate={{
                                    x: [-40, 40, -40],
                                    opacity: 0.9,
                                }}
                                transition={{
                                    repeat: Infinity,
                                    duration: 6,
                                    ease: "easeInOut",
                                }}
                                filter="url(#shadow)"
                            />
                        )}

                        {/* Towers */}
                        <rect
                            x="50"
                            y="60"
                            width="20"
                            height="200"
                            fill="#263238"
                            stroke="#0d47a1"
                            strokeWidth="2"
                            filter="url(#shadow)"
                        />
                        <rect
                            x="230"
                            y="60"
                            width="20"
                            height="200"
                            fill="#263238"
                            stroke="#0d47a1"
                            strokeWidth="2"
                            filter="url(#shadow)"
                        />

                        {/* Bridge Span */}
                        <motion.g
                            animate={{
                                y: data.bridgeState ? -100 : 0,
                                scaleY: data.bridgeState ? 1.1 : 1,
                                x: data.bridgeState ? [0, -2, 0] : 0,
                            }}
                            transition={{
                                y: {
                                    duration: 6,
                                    ease: [0.4, 0, 0.2, 1],
                                    type: "spring",
                                    stiffness: 50,
                                    damping: 25,
                                },
                                scaleY: { duration: 6, ease: "easeInOut" },
                                x: {
                                    repeat: data.bridgeState ? Infinity : 0,
                                    duration: 0.5,
                                    ease: "easeInOut",
                                },
                            }}
                        >
                            <rect
                                x="70"
                                y="160"
                                width="160"
                                height="20"
                                fill="url(#bridgeGradient)"
                                stroke="#0d47a1"
                                strokeWidth="2"
                                filter="url(#shadow)"
                            />
                            {/* Bridge Texture */}
                            {[...Array(8)].map((_, i) => (
                                <g key={i}>
                                    <path
                                        d={`M${75 + i * 20} 160 L${85 + i * 20} 180`}
                                        stroke="#0d47a1"
                                        strokeWidth="1.5"
                                    />
                                    <path
                                        d={`M${85 + i * 20} 160 L${75 + i * 20} 180`}
                                        stroke="#0d47a1"
                                        strokeWidth="1.5"
                                    />
                                </g>
                            ))}
                        </motion.g>
                    </svg>
                </Box>

                {/* Boat Indicator */}
                <BoatDetected
                    isPassing={data.currentState === 5}
                    isDetected={data.currentState === 1}
                />

                {/* Traffic Lights */}
                <Box
                    sx={{
                        display: "flex",
                        justifyContent: "space-around",
                        mt: 3,
                        mb: 3,
                    }}
                >
                    <TrafficLight
                        title="Traffic Lights"
                        icon={<TrafficIcon sx={{ mr: 0.5 }} />}
                        red={data.redLedA}
                        yellow={data.yellowLedA}
                        green={data.greenLedA}
                    />
                    <TrafficLight
                        title="Boat Lights"
                        icon={<DirectionsBoatIcon sx={{ mr: 0.5 }} />}
                        red={data.redLedB}
                        yellow={data.yellowLedB}
                        green={data.greenLedB}
                    />
                </Box>

                {/* Override Toggle Button */}
                <Box sx={{ mb: 2 }}>
                    <Button
                        variant={data.manualOverride ? "contained" : "outlined"}
                        startIcon={data.manualOverride ? <AutoModeIcon /> : <BuildIcon />}
                        onClick={toggleOverride}
                        disabled={loading}
                        sx={{
                            px: 4,
                            py: 1.5,
                            bgcolor: data.manualOverride ? "#FF9800" : "transparent",
                            color: data.manualOverride ? "#fff" : "#FF9800",
                            borderColor: "#FF9800",
                            "&:hover": { 
                                bgcolor: data.manualOverride ? "#F57C00" : "rgba(255, 152, 0, 0.1)",
                                borderColor: "#F57C00"
                            },
                            fontWeight: 600,
                        }}
                    >
                        {data.manualOverride ? "Return to Auto Mode" : "Enable Manual Override"}
                    </Button>
                </Box>

                {/* Bridge Controls */}
                <Box
                    sx={{
                        display: "flex",
                        justifyContent: "center",
                        gap: 2,
                        flexWrap: "wrap",
                    }}
                >
                    <Button
                        variant="contained"
                        startIcon={<LockOpenIcon />}
                        onClick={() => sendCommand("open")}
                        disabled={
                            loading || 
                            (!data.manualOverride && data.bridgeState) ||
                            (!data.manualOverride && data.currentState !== 0)
                        }
                        sx={{
                            px: 4,
                            py: 1.5,
                            bgcolor: "#4CAF50",
                            "&:hover": { bgcolor: "#388E3C" },
                        }}
                    >
                        Open Bridge
                    </Button>
                    <Button
                        variant="contained"
                        startIcon={<LockIcon />}
                        onClick={() => sendCommand("close")}
                        disabled={
                            loading || 
                            (!data.manualOverride && !data.bridgeState)
                        }
                        sx={{
                            px: 4,
                            py: 1.5,
                            bgcolor: "#F44336",
                            "&:hover": { bgcolor: "#D32F2F" },
                        }}
                    >
                        Close Bridge
                    </Button>
                    <Button
                        variant="contained"
                        startIcon={<RefreshIcon />}
                        onClick={() => sendCommand("clear")}
                        disabled={loading}
                        sx={{
                            px: 4,
                            py: 1.5,
                            bgcolor: "#2196F3",
                            "&:hover": { bgcolor: "#1976D2" },
                        }}
                    >
                        Clear
                    </Button>
                </Box>

                {/* Logout Button */}
                <Button
                    variant="outlined"
                    startIcon={<LogoutIcon sx={{ fontSize: "1rem" }} />}
                    onClick={handleLogout}
                    sx={{
                        position: "absolute",
                        bottom: 16,
                        right: 16,
                        fontSize: "0.8rem",
                        px: 2,
                        py: 0.5,
                        color: "#333",
                        borderColor: "#333",
                        "&:hover": { bgcolor: "#eee" },
                    }}
                >
                    Logout
                </Button>
            </Card>
        </Box>
    );
}

export default App;