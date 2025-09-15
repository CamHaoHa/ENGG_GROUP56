import React, { useState, useEffect } from "react";
import { Card, Typography, Button } from "@mui/material";
import { motion } from "framer-motion";
import io from "socket.io-client";

const SOCKET_SERVER = "http://localhost:3000"; // Update to server IP

function App() {
    const [data, setData] = useState({ state: 0, distance: 0 });
    const socket = io(SOCKET_SERVER);

    useEffect(() => {
        socket.on("update", (newData) => {
            setData(newData);
        });
        return () => socket.disconnect();
    }, [socket]);

    const sendOverride = (action) => {
        socket.emit("override", { action });
    };

    return (
        <div style={{ padding: 20 }}>
            <Card style={{ padding: 20 }}>
                <Typography variant="h5">Bridge State: {data.state}</Typography>
                <Typography>Ship Distance: {data.distance} cm</Typography>
                <motion.div
                    animate={{ y: data.state >= 4 ? -100 : 0 }} // Simple lift animation
                    transition={{ duration: 1 }}
                >
                    <div style={{ width: 200, height: 50, background: "gray" }}>
                        Bridge Model
                    </div>
                </motion.div>
                {/* Add traffic light visuals, e.g., colored divs */}
                <Button
                    variant="contained"
                    onClick={() => sendOverride("raise")}
                >
                    Manual Raise
                </Button>
            </Card>
        </div>
    );
}

export default App;
