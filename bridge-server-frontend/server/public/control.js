const state = document.getElementById("state");
const shipStatus = document.getElementById("shipStatus");
const shipState = document.getElementById("shipState");
const bridge = document.getElementById("bridge");
const openBtn = document.getElementById("openButton");
const closeBtn = document.getElementById("closeButton");
const logoutBtn = document.getElementById("logoutButton");
const clearBtn = document.getElementById("clearShipButton");
const redBtn = document.getElementById("redLedButton");
const yellowBtn = document.getElementById("yellowLedButton");
const greenBtn = document.getElementById("greenLedButton");

if (!state || !shipStatus || !shipState || !bridge || !openBtn || !closeBtn || !logoutBtn || !clearBtn || !redBtn || !yellowBtn || !greenBtn) {
    document.getElementById("fallback").style.display = "block";
} else {
    openBtn.addEventListener("click", () => controlBridge("open"));
    closeBtn.addEventListener("click", () => controlBridge("close"));
    logoutBtn.addEventListener("click", () => {
        fetch("/logout").then(() => window.location = "/logout");
    });
    clearBtn.addEventListener("click", () => {
        fetch("/api/clear-ship").then(() => {
            shipState.innerHTML = "None";
            shipStatus.classList.remove("flashing");
        });
    });
    redBtn.addEventListener("click", () => toggleLed(redBtn, "redLed"));
    yellowBtn.addEventListener("click", () => toggleLed(yellowBtn, "yellowLed"));
    greenBtn.addEventListener("click", () => toggleLed(greenBtn, "greenLed"));

    function controlBridge(action) {
        state.innerHTML = action === "open" ? "Opening" : "Closing";
        bridge.setAttribute("class", "bridge " + (action === "open" ? "opening" : "closing"));
        fetch("/api/control-bridge", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ action })
        }).then(res => {
            if (res.ok) {
                setTimeout(() => {
                    state.innerHTML = action === "open" ? "Open" : "Closed";
                    bridge.setAttribute("class", "bridge " + (action === "open" ? "open" : "closed"));
                }, 3000);
            }
        });
    }

    function toggleLed(btn, led) {
        const currentState = btn.getAttribute("data-state");
        const newState = currentState === "1" ? "0" : "1";
        fetch("/api/control-led", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ led, state: newState })
        }).then(res => {
            if (res.ok) {
                btn.setAttribute("data-state", newState);
                btn.innerHTML = `Turn ${led === "redLed" ? "Red" : led === "yellowLed" ? "Yellow" : "Green"} LED ${newState === "1" ? "Off" : "On"}`;
            }
        });
    }

    function updateState() {
        fetch("/api/state").then(res => res.json()).then(data => {
            if (data.bridgeState) {
                if (state.innerHTML !== "Open") {
                    state.innerHTML = "Opening";
                    bridge.setAttribute("class", "bridge opening");
                    setTimeout(() => {
                        state.innerHTML = "Open";
                        bridge.setAttribute("class", "bridge open");
                    }, 3000);
                }
            } else {
                if (state.innerHTML !== "Closed") {
                    state.innerHTML = "Closing";
                    bridge.setAttribute("class", "bridge closing");
                    setTimeout(() => {
                        state.innerHTML = "Closed";
                        bridge.setAttribute("class", "bridge closed");
                    }, 3000);
                }
            }
            shipState.innerHTML = data.shipDetected ? "Detected" : "None";
            if (data.shipDetected) {
                shipStatus.classList.add("flashing");
            } else {
                shipStatus.classList.remove("flashing");
            }
            redBtn.setAttribute("data-state", data.redLed ? "1" : "0");
            redBtn.innerHTML = `Red LED ${data.redLed ? "Off" : "On"}`;
            yellowBtn.setAttribute("data-state", data.yellowLed ? "1" : "0");
            yellowBtn.innerHTML = `Yellow LED ${data.yellowLed ? "Off" : "On"}`;
            greenBtn.setAttribute("data-state", data.greenLed ? "1" : "0");
            greenBtn.innerHTML = `Green LED ${data.greenLed ? "Off" : "On"}`;
        });
    }

    setInterval(updateState, 200); // Poll server for state updates
    updateState(); // Initial update
}