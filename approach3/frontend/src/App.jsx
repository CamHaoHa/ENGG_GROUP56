import React, { useState, useEffect, memo } from "react";
import {
    Card,
    Typography,
    Button,
    Box,
    CircularProgress,
    Alert,
    Dialog,
    DialogTitle,
    DialogContent,
    DialogContentText,
    DialogActions,
} from "@mui/material";
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
import WarningIcon from "@mui/icons-material/Warning";
import SensorsIcon from "@mui/icons-material/Sensors";

const API_URL = "http://192.168.4.1";
const stateNames = [
    "STATE 0: IDLE (Bridge Closed)",
    "STATE 1: Boat Detected",
    "STATE 2: Clearing Traffic",
    "STATE 2B: Traffic Clear (Confirmed)",
    "STATE 3: Opening Bridge",
    "STATE 4: Bridge Open (Yellow Warning)",
    "STATE 5: Bridge Open (Waiting for Boats)",
    "STATE 6: Stopping Boats",
    "STATE 7: Closing Bridge",
    "STATE 8: Bridge Closed (Preparing Traffic)",
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

// NEW: Boat Detection Indicator Component with Flashing Animation
const BoatDetectionIndicator = memo(
    ({ boatDetected, sensor1, sensor2, currentState }) => {
        const isDetecting = boatDetected && currentState === 0;

        return (
            <Box
                component={motion.div}
                animate={{
                    scale: isDetecting ? [1, 1.05, 1] : 1,
                    boxShadow: isDetecting
                        ? [
                              "0 0 20px rgba(255, 152, 0, 0.5)",
                              "0 0 40px rgba(255, 152, 0, 0.8)",
                              "0 0 20px rgba(255, 152, 0, 0.5)",
                          ]
                        : "0 4px 6px rgba(0,0,0,0.1)",
                }}
                transition={{
                    duration: 0.8,
                    repeat: isDetecting ? Infinity : 0,
                    ease: "easeInOut",
                }}
                sx={{
                    p: 3,
                    mb: 3,
                    borderRadius: 3,
                    bgcolor: isDetecting ? "#FF9800" : "#E0E0E0",
                    border: isDetecting
                        ? "3px solid #F57C00"
                        : "3px solid #BDBDBD",
                    display: "flex",
                    alignItems: "center",
                    justifyContent: "center",
                    gap: 2,
                    transition: "background-color 0.3s ease",
                }}
            >
                <Box
                    component={motion.div}
                    animate={{
                        rotate: isDetecting ? [0, 360] : 0,
                    }}
                    transition={{
                        duration: 2,
                        repeat: isDetecting ? Infinity : 0,
                        ease: "linear",
                    }}
                >
                    <SensorsIcon
                        sx={{
                            fontSize: "3rem",
                            color: isDetecting ? "#fff" : "#666",
                        }}
                    />
                </Box>

                <Box sx={{ textAlign: "left" }}>
                    <Typography
                        sx={{
                            fontSize: "1.5rem",
                            fontWeight: 700,
                            color: isDetecting ? "#fff" : "#666",
                            mb: 0.5,
                        }}
                    >
                        {isDetecting
                            ? "🚤 BOAT DETECTED!"
                            : "🌊 No Boat Detected"}
                    </Typography>

                    <Typography
                        sx={{
                            fontSize: "1rem",
                            color: isDetecting ? "#fff" : "#888",
                            fontWeight: 500,
                        }}
                    >
                        {isDetecting
                            ? "Waiting for continuous detection (500ms)..."
                            : "Monitoring waterway (30-50cm range)"}
                    </Typography>

                    {/* Sensor Status */}
                    <Box sx={{ mt: 1, display: "flex", gap: 2 }}>
                        <Box
                            sx={{
                                display: "flex",
                                alignItems: "center",
                                gap: 0.5,
                                px: 1.5,
                                py: 0.5,
                                borderRadius: 1,
                                bgcolor: sensor1
                                    ? "rgba(76, 175, 80, 0.9)"
                                    : "rgba(255, 255, 255, 0.2)",
                                border: sensor1
                                    ? "2px solid #2E7D32"
                                    : "2px solid rgba(255, 255, 255, 0.3)",
                            }}
                        >
                            <Box
                                sx={{
                                    width: 8,
                                    height: 8,
                                    borderRadius: "50%",
                                    bgcolor: sensor1 ? "#4CAF50" : "#999",
                                }}
                            />
                            <Typography
                                sx={{
                                    fontSize: "0.85rem",
                                    fontWeight: 600,
                                    color: sensor1
                                        ? "#fff"
                                        : isDetecting
                                        ? "#fff"
                                        : "#666",
                                }}
                            >
                                Sensor 1 {sensor1 ? "✓" : "○"}
                            </Typography>
                        </Box>

                        <Box
                            sx={{
                                display: "flex",
                                alignItems: "center",
                                gap: 0.5,
                                px: 1.5,
                                py: 0.5,
                                borderRadius: 1,
                                bgcolor: sensor2
                                    ? "rgba(76, 175, 80, 0.9)"
                                    : "rgba(255, 255, 255, 0.2)",
                                border: sensor2
                                    ? "2px solid #2E7D32"
                                    : "2px solid rgba(255, 255, 255, 0.3)",
                            }}
                        >
                            <Box
                                sx={{
                                    width: 8,
                                    height: 8,
                                    borderRadius: "50%",
                                    bgcolor: sensor2 ? "#4CAF50" : "#999",
                                }}
                            />
                            <Typography
                                sx={{
                                    fontSize: "0.85rem",
                                    fontWeight: 600,
                                    color: sensor2
                                        ? "#fff"
                                        : isDetecting
                                        ? "#fff"
                                        : "#666",
                                }}
                            >
                                Sensor 2 {sensor2 ? "✓" : "○"}
                            </Typography>
                        </Box>
                    </Box>
                </Box>
            </Box>
        );
    },
    (prevProps, nextProps) =>
        prevProps.boatDetected === nextProps.boatDetected &&
        prevProps.sensor1 === nextProps.sensor1 &&
        prevProps.sensor2 === nextProps.sensor2 &&
        prevProps.currentState === nextProps.currentState
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
        boatDetected: false, // NEW: Boat detection status
        boatSensor1: false, // NEW: Sensor 1 status
        boatSensor2: false, // NEW: Sensor 2 status
    });
    const [error, setError] = useState(null);
    const [isAuthenticated, setIsAuthenticated] = useState(
        !!localStorage.getItem("authToken")
    );
    const [logoutMessage, setLogoutMessage] = useState(null);
    const [loading, setLoading] = useState(false);
    const [authErrorCount, setAuthErrorCount] = useState(0);
    const [confirmDialog, setConfirmDialog] = useState({
        open: false,
        action: "",
        title: "",
        message: "",
    });

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
                const url = `${API_URL}/api/state?token=${encodeURIComponent(
                    token
                )}`;
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
                console.log("Boat detected:", newData.boatDetected);
                console.log("Sensor 1:", newData.boatSensor1);
                console.log("Sensor 2:", newData.boatSensor2);

                setData((prev) => {
                    if (JSON.stringify(prev) === JSON.stringify(newData)) {
                        return prev;
                    }
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
        const interval = setInterval(fetchState, 500); // Poll every 500ms for faster boat detection updates
        return () => clearInterval(interval);
    }, [isAuthenticated, authToken]);

    const sendCommand = async (action) => {
        const token = authToken;
        console.log("=== SEND COMMAND ===");
        console.log("Action:", action);
        console.log("Token:", token);

        if (!token) {
            console.error("No token available!");
            setIsAuthenticated(false);
            setError("No authentication token. Please log in.");
            return;
        }
        setLoading(true);
        try {
            const url = `${API_URL}/api/command?token=${encodeURIComponent(
                token
            )}`;
            console.log("Sending POST to:", url);

            const res = await fetch(url, {
                method: "POST",
                headers: {
                    "Content-Type": "application/json",
                    "x-auth-token": token,
                },
                body: JSON.stringify({ action }),
            });

            console.log("Response status:", res.status);

            if (res.status === 401) {
                console.error("Unauthorized response!");
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

            const responseText = await res.text();
            console.log("Response body:", responseText);

            if (!res.ok) {
                throw new Error("Command failed");
            }

            setAuthErrorCount(0);
        } catch (err) {
            console.error("Send command error:", err);
            setError("Failed to send command. Check ESP32 connection.");
        } finally {
            setLoading(false);
        }
    };

    const toggleOverride = async () => {
        const action = data.manualOverride
            ? "disableOverride"
            : "enableOverride";
        await sendCommand(action);
    };

    const handleTrafficLight = async (lightColor) => {
        if (!data.manualOverride) {
            setError("Traffic light control only available in override mode");
            return;
        }
        const actionMap = {
            red: "trafficRed",
            yellow: "trafficYellow",
            green: "trafficGreen",
        };
        const action = actionMap[lightColor];
        await sendCommand(action);
    };

    const handleBoatLight = async (lightColor) => {
        if (!data.manualOverride) {
            setError("Boat light control only available in override mode");
            return;
        }
        const actionMap = {
            red: "boatRed",
            yellow: "boatYellow",
            green: "boatGreen",
        };
        const action = actionMap[lightColor];
        await sendCommand(action);
    };

    const handleBridgeAction = (action) => {
        if (data.manualOverride) {
            const isOpen = action === "open";
            setConfirmDialog({
                open: true,
                action: action,
                title: isOpen
                    ? "⚠️ Open Bridge - Safety Confirmation"
                    : "⚠️ Close Bridge - Safety Confirmation",
                message: isOpen
                    ? "Before opening the bridge, please confirm:\n\n• All traffic has cleared the bridge\n• Boom gates are down\n• No vehicles are approaching\n• Area is safe for operation\n\nDo you want to proceed?"
                    : "Before closing the bridge, please confirm:\n\n• All boats have cleared the waterway\n• No boats are approaching\n• Bridge area is clear\n• Safe to close the bridge\n\nDo you want to proceed?",
            });
        } else {
            sendCommand(action);
        }
    };

    const handleConfirmAction = async () => {
        const action = confirmDialog.action;
        setConfirmDialog({ ...confirmDialog, open: false });
        await sendCommand(action);
    };

    const handleCancelAction = () => {
        setConfirmDialog({ ...confirmDialog, open: false });
    };

    const handleLogout = async () => {
        const token = authToken;
        try {
            const url = `${API_URL}/api/logout?token=${encodeURIComponent(
                token
            )}`;
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

    const isTransitioning = data.currentState === 3 || data.currentState === 7;
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

                {/* NEW: Boat Detection Indicator - Always visible, flashes when detecting */}
                <BoatDetectionIndicator
                    boatDetected={data.boatDetected}
                    sensor1={data.boatSensor1}
                    sensor2={data.boatSensor2}
                    currentState={data.currentState}
                />

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
                    Mode:{" "}
                    {data.manualOverride ? "Manual Override" : "Automatic"}
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
                                        d={`M${75 + i * 20} 160 L${
                                            85 + i * 20
                                        } 180`}
                                        stroke="#0d47a1"
                                        strokeWidth="1.5"
                                    />
                                    <path
                                        d={`M${85 + i * 20} 160 L${
                                            75 + i * 20
                                        } 180`}
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
                        startIcon={
                            data.manualOverride ? (
                                <AutoModeIcon />
                            ) : (
                                <BuildIcon />
                            )
                        }
                        onClick={toggleOverride}
                        disabled={loading}
                        sx={{
                            px: 4,
                            py: 1.5,
                            bgcolor: data.manualOverride
                                ? "#FF9800"
                                : "transparent",
                            color: data.manualOverride ? "#fff" : "#FF9800",
                            borderColor: "#FF9800",
                            "&:hover": {
                                bgcolor: data.manualOverride
                                    ? "#F57C00"
                                    : "rgba(255, 152, 0, 0.1)",
                                borderColor: "#F57C00",
                            },
                            fontWeight: 600,
                        }}
                    >
                        {data.manualOverride
                            ? "Return to Auto Mode"
                            : "Enable Manual Override"}
                    </Button>
                </Box>

                {/* Vehicle Traffic Light Controls - Only in Override Mode */}
                {data.manualOverride && (
                    <Box
                        sx={{
                            mb: 2,
                            p: 3,
                            bgcolor: "#FFF3E0",
                            borderRadius: 2,
                            border: "2px solid #FF9800",
                        }}
                    >
                        <Typography
                            sx={{
                                fontSize: "1.2rem",
                                fontWeight: 600,
                                mb: 2,
                                color: "#E65100",
                                textAlign: "center",
                            }}
                        >
                            🚗 Vehicle Traffic Light Control
                        </Typography>
                        <Box
                            sx={{
                                display: "flex",
                                justifyContent: "center",
                                alignItems: "center",
                                gap: 3,
                                flexWrap: "wrap",
                            }}
                        >
                            {/* Visual Traffic Light Display */}
                            <Box sx={{ textAlign: "center" }}>
                                <Typography
                                    sx={{
                                        fontSize: "1rem",
                                        mb: 1,
                                        fontWeight: 600,
                                    }}
                                >
                                    Current Status
                                </Typography>
                                <Box
                                    sx={{
                                        display: "flex",
                                        flexDirection: "column",
                                        alignItems: "center",
                                        bgcolor: "#333",
                                        p: 2,
                                        borderRadius: 2,
                                        gap: 1,
                                        width: 80,
                                        boxShadow: 3,
                                    }}
                                >
                                    <Box
                                        sx={{
                                            width: 50,
                                            height: 50,
                                            borderRadius: "50%",
                                            bgcolor: data.redLedA
                                                ? "#F44336"
                                                : "#444",
                                            border: data.redLedA
                                                ? "4px solid #B71C1C"
                                                : "4px solid #333",
                                            boxShadow: data.redLedA
                                                ? "0 0 20px #F44336"
                                                : "none",
                                            transition: "all 0.3s ease",
                                        }}
                                    />
                                    <Box
                                        sx={{
                                            width: 50,
                                            height: 50,
                                            borderRadius: "50%",
                                            bgcolor: data.yellowLedA
                                                ? "#FFCA28"
                                                : "#444",
                                            border: data.yellowLedA
                                                ? "4px solid #F57F17"
                                                : "4px solid #333",
                                            boxShadow: data.yellowLedA
                                                ? "0 0 20px #FFCA28"
                                                : "none",
                                            transition: "all 0.3s ease",
                                        }}
                                    />
                                    <Box
                                        sx={{
                                            width: 50,
                                            height: 50,
                                            borderRadius: "50%",
                                            bgcolor: data.greenLedA
                                                ? "#4CAF50"
                                                : "#444",
                                            border: data.greenLedA
                                                ? "4px solid #2E7D32"
                                                : "4px solid #333",
                                            boxShadow: data.greenLedA
                                                ? "0 0 20px #4CAF50"
                                                : "none",
                                            transition: "all 0.3s ease",
                                        }}
                                    />
                                </Box>
                            </Box>

                            {/* Control Buttons */}
                            <Box
                                sx={{
                                    display: "flex",
                                    flexDirection: "column",
                                    gap: 1.5,
                                }}
                            >
                                <Button
                                    variant="contained"
                                    onClick={() => handleTrafficLight("red")}
                                    disabled={loading || data.redLedA}
                                    sx={{
                                        px: 4,
                                        py: 1.5,
                                        bgcolor: "#F44336",
                                        color: "#fff",
                                        fontSize: "1rem",
                                        fontWeight: 600,
                                        "&:hover": { bgcolor: "#D32F2F" },
                                        "&:disabled": {
                                            bgcolor: "#ffcdd2",
                                            color: "#fff",
                                        },
                                        minWidth: 180,
                                    }}
                                >
                                    {data.redLedA
                                        ? "🔴 RED (Active)"
                                        : "Set RED"}
                                </Button>
                                <Button
                                    variant="contained"
                                    onClick={() => handleTrafficLight("yellow")}
                                    disabled={loading || data.yellowLedA}
                                    sx={{
                                        px: 4,
                                        py: 1.5,
                                        bgcolor: "#FFCA28",
                                        color: "#000",
                                        fontSize: "1rem",
                                        fontWeight: 600,
                                        "&:hover": { bgcolor: "#FFA000" },
                                        "&:disabled": {
                                            bgcolor: "#fff9c4",
                                            color: "#666",
                                        },
                                        minWidth: 180,
                                    }}
                                >
                                    {data.yellowLedA
                                        ? "🟡 YELLOW (Active)"
                                        : "Set YELLOW"}
                                </Button>
                                <Button
                                    variant="contained"
                                    onClick={() => handleTrafficLight("green")}
                                    disabled={loading || data.greenLedA}
                                    sx={{
                                        px: 4,
                                        py: 1.5,
                                        bgcolor: "#4CAF50",
                                        color: "#fff",
                                        fontSize: "1rem",
                                        fontWeight: 600,
                                        "&:hover": { bgcolor: "#388E3C" },
                                        "&:disabled": {
                                            bgcolor: "#c8e6c9",
                                            color: "#fff",
                                        },
                                        minWidth: 180,
                                    }}
                                >
                                    {data.greenLedA
                                        ? "🟢 GREEN (Active)"
                                        : "Set GREEN"}
                                </Button>
                            </Box>
                        </Box>
                        <Typography
                            sx={{
                                fontSize: "0.9rem",
                                color: "#666",
                                textAlign: "center",
                                mt: 2,
                                fontStyle: "italic",
                            }}
                        >
                            💡 Click any light to change vehicle traffic signal
                            • Active light is disabled
                        </Typography>
                    </Box>
                )}

                {/* Boat Traffic Light Controls - Only in Override Mode */}
                {data.manualOverride && (
                    <Box
                        sx={{
                            mb: 2,
                            p: 3,
                            bgcolor: "#E3F2FD",
                            borderRadius: 2,
                            border: "2px solid #2196F3",
                        }}
                    >
                        <Typography
                            sx={{
                                fontSize: "1.2rem",
                                fontWeight: 600,
                                mb: 2,
                                color: "#0D47A1",
                                textAlign: "center",
                            }}
                        >
                            ⛵ Boat Traffic Light Control
                        </Typography>
                        <Box
                            sx={{
                                display: "flex",
                                justifyContent: "center",
                                alignItems: "center",
                                gap: 3,
                                flexWrap: "wrap",
                            }}
                        >
                            {/* Visual Boat Light Display */}
                            <Box sx={{ textAlign: "center" }}>
                                <Typography
                                    sx={{
                                        fontSize: "1rem",
                                        mb: 1,
                                        fontWeight: 600,
                                    }}
                                >
                                    Current Status
                                </Typography>
                                <Box
                                    sx={{
                                        display: "flex",
                                        flexDirection: "column",
                                        alignItems: "center",
                                        bgcolor: "#1565C0",
                                        p: 2,
                                        borderRadius: 2,
                                        gap: 1,
                                        width: 80,
                                        boxShadow: 3,
                                    }}
                                >
                                    <Box
                                        sx={{
                                            width: 50,
                                            height: 50,
                                            borderRadius: "50%",
                                            bgcolor: data.redLedB
                                                ? "#F44336"
                                                : "#0D47A1",
                                            border: data.redLedB
                                                ? "4px solid #B71C1C"
                                                : "4px solid #0D47A1",
                                            boxShadow: data.redLedB
                                                ? "0 0 20px #F44336"
                                                : "none",
                                            transition: "all 0.3s ease",
                                        }}
                                    />
                                    <Box
                                        sx={{
                                            width: 50,
                                            height: 50,
                                            borderRadius: "50%",
                                            bgcolor: data.yellowLedB
                                                ? "#FFCA28"
                                                : "#0D47A1",
                                            border: data.yellowLedB
                                                ? "4px solid #F57F17"
                                                : "4px solid #0D47A1",
                                            boxShadow: data.yellowLedB
                                                ? "0 0 20px #FFCA28"
                                                : "none",
                                            transition: "all 0.3s ease",
                                        }}
                                    />
                                    <Box
                                        sx={{
                                            width: 50,
                                            height: 50,
                                            borderRadius: "50%",
                                            bgcolor: data.greenLedB
                                                ? "#4CAF50"
                                                : "#0D47A1",
                                            border: data.greenLedB
                                                ? "4px solid #2E7D32"
                                                : "4px solid #0D47A1",
                                            boxShadow: data.greenLedB
                                                ? "0 0 20px #4CAF50"
                                                : "none",
                                            transition: "all 0.3s ease",
                                        }}
                                    />
                                </Box>
                            </Box>

                            {/* Control Buttons */}
                            <Box
                                sx={{
                                    display: "flex",
                                    flexDirection: "column",
                                    gap: 1.5,
                                }}
                            >
                                <Button
                                    variant="contained"
                                    onClick={() => handleBoatLight("red")}
                                    disabled={loading || data.redLedB}
                                    sx={{
                                        px: 4,
                                        py: 1.5,
                                        bgcolor: "#F44336",
                                        color: "#fff",
                                        fontSize: "1rem",
                                        fontWeight: 600,
                                        "&:hover": { bgcolor: "#D32F2F" },
                                        "&:disabled": {
                                            bgcolor: "#ffcdd2",
                                            color: "#fff",
                                        },
                                        minWidth: 180,
                                    }}
                                >
                                    {data.redLedB
                                        ? "🔴 RED (Active)"
                                        : "Set RED"}
                                </Button>
                                <Button
                                    variant="contained"
                                    onClick={() => handleBoatLight("yellow")}
                                    disabled={loading || data.yellowLedB}
                                    sx={{
                                        px: 4,
                                        py: 1.5,
                                        bgcolor: "#FFCA28",
                                        color: "#000",
                                        fontSize: "1rem",
                                        fontWeight: 600,
                                        "&:hover": { bgcolor: "#FFA000" },
                                        "&:disabled": {
                                            bgcolor: "#fff9c4",
                                            color: "#666",
                                        },
                                        minWidth: 180,
                                    }}
                                >
                                    {data.yellowLedB
                                        ? "🟡 YELLOW (Active)"
                                        : "Set YELLOW"}
                                </Button>
                                <Button
                                    variant="contained"
                                    onClick={() => handleBoatLight("green")}
                                    disabled={loading || data.greenLedB}
                                    sx={{
                                        px: 4,
                                        py: 1.5,
                                        bgcolor: "#4CAF50",
                                        color: "#fff",
                                        fontSize: "1rem",
                                        fontWeight: 600,
                                        "&:hover": { bgcolor: "#388E3C" },
                                        "&:disabled": {
                                            bgcolor: "#c8e6c9",
                                            color: "#fff",
                                        },
                                        minWidth: 180,
                                    }}
                                >
                                    {data.greenLedB
                                        ? "🟢 GREEN (Active)"
                                        : "Set GREEN"}
                                </Button>
                            </Box>
                        </Box>
                        <Typography
                            sx={{
                                fontSize: "0.9rem",
                                color: "#666",
                                textAlign: "center",
                                mt: 2,
                                fontStyle: "italic",
                            }}
                        >
                            💡 Click any light to change boat traffic signal •
                            Active light is disabled
                        </Typography>
                    </Box>
                )}

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
                        onClick={() => handleBridgeAction("open")}
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
                        onClick={() => handleBridgeAction("close")}
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

                {/* Safety Confirmation Dialog */}
                <Dialog
                    open={confirmDialog.open}
                    onClose={handleCancelAction}
                    maxWidth="sm"
                    fullWidth
                >
                    <DialogTitle
                        sx={{
                            bgcolor: "#FFF3E0",
                            color: "#E65100",
                            display: "flex",
                            alignItems: "center",
                            gap: 1,
                        }}
                    >
                        <WarningIcon sx={{ fontSize: "2rem" }} />
                        {confirmDialog.title}
                    </DialogTitle>
                    <DialogContent sx={{ mt: 2 }}>
                        <DialogContentText
                            sx={{
                                whiteSpace: "pre-line",
                                fontSize: "1.1rem",
                                color: "#333",
                            }}
                        >
                            {confirmDialog.message}
                        </DialogContentText>
                    </DialogContent>
                    <DialogActions sx={{ p: 2, gap: 1 }}>
                        <Button
                            onClick={handleCancelAction}
                            variant="outlined"
                            sx={{
                                px: 3,
                                py: 1,
                                color: "#666",
                                borderColor: "#666",
                                "&:hover": {
                                    bgcolor: "#f5f5f5",
                                    borderColor: "#333",
                                },
                            }}
                        >
                            Cancel
                        </Button>
                        <Button
                            onClick={handleConfirmAction}
                            variant="contained"
                            color="warning"
                            autoFocus
                            sx={{
                                px: 3,
                                py: 1,
                                bgcolor: "#FF9800",
                                "&:hover": { bgcolor: "#F57C00" },
                            }}
                        >
                            Confirm & Proceed
                        </Button>
                    </DialogActions>
                </Dialog>
            </Card>
        </Box>
    );
}

export default App;
