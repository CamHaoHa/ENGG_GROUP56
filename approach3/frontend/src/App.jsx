import React, { useState, useEffect, memo } from "react";
import {
    Card,
    Typography,
    Button,
    Box,
    CircularProgress,
    Alert,
    Chip,
    Dialog,
    DialogTitle,
    DialogContent,
    DialogContentText,
    DialogActions,
    ButtonGroup,
    Paper,
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
import SensorsIcon from "@mui/icons-material/Sensors";
import VolumeUpIcon from "@mui/icons-material/VolumeUp";
import ToggleOffIcon from "@mui/icons-material/ToggleOff";
import ToggleOnIcon from "@mui/icons-material/ToggleOn";
import WarningIcon from "@mui/icons-material/Warning";

const API_URL = "http://192.168.4.1";

const stateNames = [
    "0: Idle (Traffic Flowing)",
    "1: Boat Detected",
    "2: Clearing Traffic",
    "3: Traffic Clear",
    "4: Opening Bridge",
    "5: Bridge Fully Open (Yellow Warning)",
    "6: Bridge Open (Boats Passing)",
    "7: Stopping Boats",
    "8: Closing Bridge",
    "9: Bridge Closed (Preparing Traffic)",
];

const overrideStepNames = [
    "Override Ready - Awaiting Commands",
    "Step 1: Clearing Traffic...",
    "Step 2: Traffic Cleared - Ready to Open",
    "Step 3: Bridge Opening...",
    "Step 4: Bridge Open - Monitoring",
    "Step 5: Bridge Closing...",
];

// ==================== Reusable Components ====================
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
    ({ isDetected, isClearing }) => (
        <motion.div
            animate={{
                backgroundColor: isDetected ? "#FFCA28" : "#E0E0E0",
                opacity: isDetected ? [1, 0.6, 1] : 1,
            }}
            transition={
                isDetected
                    ? { duration: 0.6, repeat: Infinity, ease: "easeInOut" }
                    : {}
            }
            style={{
                padding: "16px",
                borderRadius: "8px",
                textAlign: "center",
                fontSize: "1.2rem",
                fontWeight: 600,
                color: isDetected ? "#000" : "#555",
                marginBottom: "16px",
                display: "flex",
                alignItems: "center",
                justifyContent: "center",
                gap: "8px",
            }}
        >
            <SensorsIcon />
            Boat: {isDetected ? "Detected" : "None"}
            {isClearing && (
                <VolumeUpIcon sx={{ animation: "pulse 1s infinite" }} />
            )}
        </motion.div>
    ),
    (prevProps, nextProps) =>
        prevProps.isDetected === nextProps.isDetected &&
        prevProps.isClearing === nextProps.isClearing
);

const ManualControlPanel = ({ manualOverride, sendCommand, trafficLights }) =>
    manualOverride && (
        <Paper
            sx={{
                p: 2,
                mb: 2,
                bgcolor: "#fff3e0",
                border: "2px solid #ff9800",
            }}
        >
            <Typography variant="h6" sx={{ mb: 2, color: "#e65100" }}>
                <TrafficIcon sx={{ mr: 1, verticalAlign: "middle" }} />
                Manual Traffic Light Control
            </Typography>
            <Box
                sx={{
                    display: "flex",
                    gap: 2,
                    justifyContent: "center",
                    flexWrap: "wrap",
                }}
            >
                <ButtonGroup variant="outlined" size="small">
                    {["red", "yellow", "green"].map((color) => (
                        <Button
                            key={`traffic-${color}`}
                            onClick={() =>
                                sendCommand("setTrafficLight", {
                                    light: "A",
                                    color,
                                })
                            }
                            sx={{
                                bgcolor:
                                    (trafficLights.redLedA && color === "red") ||
                                    (trafficLights.yellowLedA && color === "yellow") ||
                                    (trafficLights.greenLedA && color === "green")
                                        ? color === "red"
                                            ? "#ffcdd2"
                                            : color === "yellow"
                                                ? "#fff9c4"
                                                : "#c8e6c9"
                                        : "white",
                            }}
                        >
                            Traffic {color.toUpperCase()}
                        </Button>
                    ))}
                </ButtonGroup>
                <ButtonGroup variant="outlined" size="small">
                    {["red", "yellow", "green"].map((color) => (
                        <Button
                            key={`boat-${color}`}
                            onClick={() =>
                                sendCommand("setTrafficLight", {
                                    light: "B",
                                    color,
                                })
                            }
                            sx={{
                                bgcolor:
                                    (trafficLights.redLedB && color === "red") ||
                                    (trafficLights.yellowLedB && color === "yellow") ||
                                    (trafficLights.greenLedB && color === "green")
                                        ? color === "red"
                                            ? "#ffcdd2"
                                            : color === "yellow"
                                                ? "#fff9c4"
                                                : "#c8e6c9"
                                        : "white",
                            }}
                        >
                            Boat {color.toUpperCase()}
                        </Button>
                    ))}
                </ButtonGroup>
            </Box>
            <Box
                sx={{
                    mt: 2,
                    display: "flex",
                    gap: 1,
                    justifyContent: "center",
                }}
            >
                <Button
                    size="small"
                    variant="outlined"
                    onClick={() =>
                        sendCommand("boomGateControl", { action: "up" })
                    }
                >
                    Boom Gate UP
                </Button>
                <Button
                    size="small"
                    variant="outlined"
                    onClick={() =>
                        sendCommand("boomGateControl", { action: "down" })
                    }
                >
                    Boom Gate DOWN
                </Button>
                <Button
                    size="small"
                    variant="outlined"
                    onClick={() =>
                        sendCommand("speakerControl", { action: "toggle" })
                    }
                >
                    Toggle Speaker
                </Button>
            </Box>
        </Paper>
    );

const BridgeControls = ({
                            handleOpenBridge,
                            handleCloseBridge,
                            sendCommand,
                            loading,
                            manualOverride,
                            bridgeState,
                            currentState,
                        }) => (
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
            onClick={handleOpenBridge}
            disabled={
                loading ||
                (!manualOverride && bridgeState) ||
                (!manualOverride && currentState !== 0)
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
            onClick={handleCloseBridge}
            disabled={loading || (!manualOverride && !bridgeState)}
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
);

const ConfirmationDialog = ({ open, title, message, onClose }) => (
    <Dialog open={open} onClose={() => onClose(false)} maxWidth="sm" fullWidth>
        <DialogTitle
            sx={{
                bgcolor: "#ff9800",
                color: "white",
                display: "flex",
                alignItems: "center",
                gap: 1,
            }}
        >
            <WarningIcon />
            {title}
        </DialogTitle>
        <DialogContent sx={{ mt: 2 }}>
            <DialogContentText
                sx={{ whiteSpace: "pre-line", fontSize: "1.1rem" }}
            >
                {message}
            </DialogContentText>
        </DialogContent>
        <DialogActions sx={{ p: 2 }}>
            <Button onClick={() => onClose(false)} variant="outlined">
                Cancel
            </Button>
            <Button
                onClick={() => onClose(true)}
                variant="contained"
                color="warning"
                autoFocus
            >
                Confirm - I Verify Safety
            </Button>
        </DialogActions>
    </Dialog>
);

// ==================== Custom Hook for API Interaction ====================
const useBridgeState = (initialToken) => {
    const [authToken, setAuthToken] = useState(
        initialToken || localStorage.getItem("authToken") || ""
    );
    const [data, setData] = useState({
        currentState: 0,
        bridgeState: false,
        manualOverride: false,
        overrideStep: 0,
        redLedA: false,
        yellowLedA: false,
        greenLedA: false,
        redLedB: false,
        yellowLedB: false,
        greenLedB: false,
        boatDetected: false,
        limitTop: false,
        limitBottom: false,
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
                setData((prev) =>
                    JSON.stringify(prev) === JSON.stringify(newData)
                        ? prev
                        : newData
                );
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

    const sendCommand = async (action, params = {}) => {
        if (!authToken) {
            setIsAuthenticated(false);
            setError("No authentication token. Please log in.");
            return;
        }

        setLoading(true);
        try {
            const url = `${API_URL}/api/command?token=${encodeURIComponent(authToken)}`;
            const res = await fetch(url, {
                method: "POST",
                headers: {
                    "Content-Type": "application/json",
                    "x-auth-token": authToken,
                },
                body: JSON.stringify({ action, ...params }),
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

            if (!res.ok) throw new Error("Command failed");
            setAuthErrorCount(0);
        } catch (err) {
            setError("Failed to send command. Check ESP32 connection.");
        } finally {
            setLoading(false);
        }
    };

    const logout = async () => {
        try {
            const url = `${API_URL}/api/logout?token=${encodeURIComponent(authToken)}`;
            await fetch(url, {
                headers: { "x-auth-token": authToken },
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

    return {
        authToken,
        setAuthToken,
        data,
        error,
        setError,
        isAuthenticated,
        setIsAuthenticated,
        logoutMessage,
        loading,
        sendCommand,
        logout,
    };
};

// ==================== Main Component ====================
function App() {
    const {
        authToken,
        setAuthToken,
        data,
        error,
        setError,
        isAuthenticated,
        setIsAuthenticated,
        logoutMessage,
        loading,
        sendCommand,
        logout,
    } = useBridgeState();

    const [confirmDialog, setConfirmDialog] = useState({
        open: false,
        title: "",
        message: "",
        action: null,
    });

    const showConfirmDialog = (title, message, action) =>
        setConfirmDialog({ open: true, title, message, action });

    const handleConfirmClose = (confirmed) => {
        if (confirmed && confirmDialog.action) {
            confirmDialog.action();
        }
        setConfirmDialog({ open: false, title: "", message: "", action: null });
    };

    const toggleOverride = () =>
        sendCommand(data.manualOverride ? "disableOverride" : "enableOverride");

    const handleOpenBridge = () =>
        data.manualOverride
            ? showConfirmDialog(
                "Confirm Bridge Opening",
                "⚠️ SAFETY CHECK:\n\n1. Ensure traffic lights are RED\n2. Confirm traffic is cleared from bridge\n3. Boom gates are DOWN\n\nDo you confirm it is SAFE to open the bridge?",
                () => sendCommand("manualOpen")
            )
            : sendCommand("open");

    const handleCloseBridge = () =>
        data.manualOverride
            ? showConfirmDialog(
                "Confirm Bridge Closing",
                "⚠️ SAFETY CHECK:\n\n1. Ensure no boats are under the bridge\n2. Boat traffic lights show RED/YELLOW\n\nDo you confirm it is SAFE to close the bridge?",
                () => sendCommand("manualClose")
            )
            : sendCommand("close");

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

    const bridgeLabel = data.bridgeState ? "Open" : "Closed";
    const isClearing = data.currentState === 2 || data.overrideStep === 1;
    const isBoatVisible =
        data.boatDetected || data.currentState === 6 || data.overrideStep === 4;

    return (
        <Box
            sx={{
                p: { xs: 2, md: 4 },
                maxWidth: 700,
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

                {data.manualOverride && (
                    <Alert
                        severity="warning"
                        sx={{ mb: 2, fontWeight: 600 }}
                        icon={<BuildIcon />}
                    >
                        <strong>MANUAL OVERRIDE ACTIVE</strong>
                        <br />
                        {overrideStepNames[data.overrideStep]}
                    </Alert>
                )}

                <Box
                    sx={{
                        display: "flex",
                        gap: 1,
                        mb: 2,
                        flexWrap: "wrap",
                        justifyContent: "center",
                    }}
                >
                    <Chip
                        icon={data.limitTop ? <ToggleOnIcon /> : <ToggleOffIcon />}
                        label="Top Limit"
                        color={data.limitTop ? "success" : "default"}
                        size="small"
                    />
                    <Chip
                        icon={data.limitBottom ? <ToggleOnIcon /> : <ToggleOffIcon />}
                        label="Bottom Limit"
                        color={data.limitBottom ? "success" : "default"}
                        size="small"
                    />
                    <Chip
                        icon={<SensorsIcon />}
                        label={data.boatDetected ? "Boat Detected" : "No Boat"}
                        color={data.boatDetected ? "warning" : "default"}
                        size="small"
                    />
                    {isClearing && (
                        <Chip
                            icon={<VolumeUpIcon />}
                            label="Warning Active"
                            color="error"
                            size="small"
                        />
                    )}
                </Box>

                <Typography
                    variant="h6"
                    sx={{ fontSize: "1.4rem", color: "#444", mb: 1 }}
                >
                    State: {stateNames[data.currentState] || "Unknown"}
                </Typography>
                <Typography sx={{ fontSize: "1.2rem", color: "#666", mb: 1 }}>
                    Bridge: {bridgeLabel}
                </Typography>
                <Typography sx={{ fontSize: "1rem", color: "#888", mb: 2 }}>
                    Mode: {data.manualOverride ? "Manual Override" : "Automatic"}
                </Typography>

                {loading && <CircularProgress size={24} sx={{ mb: 2 }} />}

                <ManualControlPanel
                    manualOverride={data.manualOverride}
                    sendCommand={sendCommand}
                    trafficLights={data}
                />

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
                            <linearGradient id="skyGradient" x1="0" y1="0" x2="0" y2="1">
                                <stop offset="0%" stopColor="#0277bd" />
                                <stop offset="100%" stopColor="#b3e5fc" />
                            </linearGradient>
                            <linearGradient id="bridgeGradient" x1="0" y1="0" x2="1" y2="0">
                                <stop offset="0%" stopColor="#455a64" />
                                <stop offset="100%" stopColor="#78909c" />
                            </linearGradient>
                            <filter id="shadow" x="-20%" y="-20%" width="140%" height="140%">
                                <feDropShadow
                                    dx="4"
                                    dy="4"
                                    stdDeviation="6"
                                    floodColor="#000000"
                                    floodOpacity="0.4"
                                />
                            </filter>
                        </defs>
                        <rect x="0" y="0" width="300" height="170" fill="url(#skyGradient)" />

                        <motion.g
                            animate={{ y: [0, -7, 0] }}
                            transition={{ repeat: Infinity, duration: 4, ease: "easeInOut" }}
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

                        {isBoatVisible && (
                            <motion.path
                                d="M100 210 L160 210 L170 230 L90 230 Z M110 210 L140 190 L150 210 Z"
                                fill="#212121"
                                animate={{ x: [-40, 40, -40], opacity: 0.9 }}
                                transition={{
                                    repeat: Infinity,
                                    duration: 6,
                                    ease: "easeInOut",
                                }}
                                filter="url(#shadow)"
                            />
                        )}

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

                <BoatDetected isDetected={data.boatDetected} isClearing={isClearing} />

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

                <Box sx={{ mb: 2 }}>
                    <Button
                        variant={data.manualOverride ? "contained" : "outlined"}
                        startIcon={
                            data.manualOverride ? <AutoModeIcon /> : <BuildIcon />
                        }
                        onClick={toggleOverride}
                        disabled={loading}
                        sx={{
                            px: 4,
                            py: 1.5,
                            bgcolor: data.manualOverride ? "#FF9800" : "transparent",
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

                <BridgeControls
                    handleOpenBridge={handleOpenBridge}
                    handleCloseBridge={handleCloseBridge}
                    sendCommand={sendCommand}
                    loading={loading}
                    manualOverride={data.manualOverride}
                    bridgeState={data.bridgeState}
                    currentState={data.currentState}
                />

                <Button
                    variant="outlined"
                    startIcon={<LogoutIcon sx={{ fontSize: "1rem" }} />}
                    onClick={logout}
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

            <ConfirmationDialog
                open={confirmDialog.open}
                title={confirmDialog.title}
                message={confirmDialog.message}
                onClose={handleConfirmClose}
            />
        </Box>
    );
}

export default App;