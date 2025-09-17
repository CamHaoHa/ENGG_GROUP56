import React, { useState } from "react";
import { Card, Typography, TextField, Button, Box } from "@mui/material";

const API_URL = "http://192.168.4.1"; // ESP32 AP IP

function Login({ setIsAuthenticated, setError }) {
    const [username, setUsername] = useState("");
    const [password, setPassword] = useState("");
    const [localError, setLocalError] = useState(null);

    const handleSubmit = (e) => {
        e.preventDefault();
        fetch(`${API_URL}/api/login`, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ username, password }),
        })
            .then((res) => {
                if (!res.ok) throw new Error("Invalid credentials");
                return res.json();
            })
            .then((data) => {
                if (data.success) {
                    localStorage.setItem("isAuthenticated", "true");
                    setIsAuthenticated(true);
                    setError(null);
                    setLocalError(null);
                } else {
                    setLocalError(data.message || "Invalid credentials");
                }
            })
            .catch((err) => {
                console.error("Login error:", err);
                setLocalError("Failed to connect to ESP32");
            });
    };

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
                    ESP32 Bridge Login
                </Typography>
                {localError && (
                    <Typography color="error" sx={{ mb: 2 }}>
                        {localError} <a href="/login">Try again</a>
                    </Typography>
                )}
                <Box
                    component="form"
                    onSubmit={handleSubmit}
                    sx={{ width: 200, mx: "auto", my: 2 }}
                >
                    <TextField
                        label="Username"
                        value={username}
                        onChange={(e) => setUsername(e.target.value)}
                        fullWidth
                        margin="normal"
                        sx={{ input: { fontSize: "1rem", p: 1 } }}
                        required
                    />
                    <TextField
                        label="Password"
                        type="password"
                        value={password}
                        onChange={(e) => setPassword(e.target.value)}
                        fullWidth
                        margin="normal"
                        sx={{ input: { fontSize: "1rem", p: 1 } }}
                        required
                    />
                    <Button
                        type="submit"
                        variant="contained"
                        sx={{
                            mt: 1,
                            px: 3,
                            py: 1,
                            fontSize: "1rem",
                            background: "#4CAF50",
                            color: "#fff",
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
