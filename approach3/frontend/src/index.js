import React from "react";
import { createRoot } from "react-dom/client";
import App from "./App";

// Get the root element
const container = document.getElementById("root");
const root = createRoot(container);

// Render the app without StrictMode for development stability
root.render(<App />);
