import React, { useState } from "react";
import { Card, Typography, TextField, Button, Box } from "@mui/material";

const API_URL = "http://192.168.4.1";

function Login({ setIsAuthenticated, setError, setAuthToken }) {
    // *** UPDATED: Accept setAuthToken prop ***
    const [username, setUsername] = useState("");
    const [password, setPassword] = useState("");
    const [localError, setLocalError] = useState(null);
    const handleSubmit = async (e) => {
        e.preventDefault();
        try {
            console.log("Login: Sending POST to /api/login with:", {
                username,
                password,
            });
            const res = await fetch(`${API_URL}/api/login`, {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({ username, password }),
            });
            console.log("Login: Response status:", res.status);
            const data = await res.json(); // Await body always (works for 401)
            console.log("Login: Response data:", data);
            if (data.success) {
                console.log("Login: Storing authToken:", data.token);
                localStorage.setItem("authToken", data.token);
                setAuthToken(data.token);
                console.log(
                    "Login: localStorage.authToken after set:",
                    localStorage.getItem("authToken")
                );
                setIsAuthenticated(true);
                setError(null);
                setLocalError(null);
            } else {
                setLocalError(
                    data.message || data.error || "Invalid credentials"
                );
            }
        } catch (err) {
            console.log("Login: Error:", err.message);
            setLocalError(
                "Failed to connect to ESP32. Check WiFi and ensure device is on."
            );
        }
    };

    return (
        <Box
            sx={{
                p: { xs: 2, sm: 4 },
                maxWidth: 400,
                mx: "auto",
                bgcolor: "#F3F4F6",
                borderRadius: 2,
                boxShadow: 3,
                mt: 8,
            }}
        >
            <Card sx={{ p: 3, bgcolor: "#fff", borderRadius: 2 }}>
                <Typography
                    variant="h4"
                    sx={{
                        fontSize: "1.8rem",
                        color: "#fff",
                        bgcolor: "#1F2937",
                        p: 2,
                        borderRadius: 1,
                        textAlign: "center",
                        mb: 2,
                    }}
                >
                    ESP32 Bridge Login
                </Typography>
                {localError && (
                    <Typography
                        color="error"
                        sx={{ mb: 2, textAlign: "center", fontSize: "1.1rem" }}
                    >
                        {localError}{" "}
                        <a href="/" className="underline text-blue-500">
                            Try again
                        </a>
                    </Typography>
                )}
                <Box
                    component="form"
                    onSubmit={handleSubmit}
                    sx={{ display: "flex", flexDirection: "column", gap: 2 }}
                >
                    <TextField
                        label="Username"
                        value={username}
                        onChange={(e) => setUsername(e.target.value)}
                        fullWidth
                        required
                        sx={{ input: { fontSize: "1.1rem", p: 1.5 } }}
                    />
                    <TextField
                        label="Password"
                        type="password"
                        value={password}
                        onChange={(e) => setPassword(e.target.value)}
                        fullWidth
                        required
                        sx={{ input: { fontSize: "1.1rem", p: 1.5 } }}
                    />
                    <Button
                        type="submit"
                        variant="contained"
                        sx={{
                            py: 1.5,
                            fontSize: "1.1rem",
                            bgcolor: "#10B981",
                            "&:hover": { bgcolor: "#059669" },
                        }}
                    >
                        Login
                    </Button>
                </Box>
            </Card>
        </Box>
    );
}

export default Login;
