const stateSpan = document.getElementById("state");
const bridgeStateSpan = document.getElementById("bridgeState");
const bridge = document.getElementById("bridge");
const openBtn = document.getElementById("openButton");
const closeBtn = document.getElementById("closeButton");
const clearBtn = document.getElementById("clearButton");
const logoutBtn = document.getElementById("logoutButton");
const trafficRed = document.getElementById("trafficRed");
const trafficYellow = document.getElementById("trafficYellow");
const trafficGreen = document.getElementById("trafficGreen");
const boatRed = document.getElementById("boatRed");
const boatYellow = document.getElementById("boatYellow");
const boatGreen = document.getElementById("boatGreen");

if (!stateSpan || !bridgeStateSpan || !bridge || !openBtn || !closeBtn || !clearBtn || !logoutBtn || !trafficRed || !trafficYellow || !trafficGreen || !boatRed || !boatYellow || !boatGreen) {
    document.getElementById("fallback").style.display = "block";
    console.error("UI elements missing: check stateSpan, bridgeStateSpan, bridge, openBtn, closeBtn, clearBtn, logoutBtn, trafficRed, trafficYellow, trafficGreen, boatRed, boatYellow, boatGreen");
} else {
    console.log("UI elements initialized successfully");

    openBtn.addEventListener("click", () => sendCommand("open"));
    closeBtn.addEventListener("click", () => sendCommand("close"));
    clearBtn.addEventListener("click", () => sendCommand("clear"));
    logoutBtn.addEventListener("click", () => {
        console.log("Logging out");
        fetch("/logout").then(() => window.location = "/logout").catch(err => console.error(`Logout error: ${err}`));
    });

    function sendCommand(action) {
        console.log(`Sending command: ${action}`);
        fetch("/api/control", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ action })
        }).then(res => {
            if (res.ok) {
                console.log(`Command ${action} sent successfully`);
                if (action === "open" || action === "close") {
                    bridgeStateSpan.innerHTML = action === "open" ? "Opening" : "Closing";
                    bridge.setAttribute("class", "bridge " + (action === "open" ? "opening" : "closing"));
                    setTimeout(() => {
                        bridgeStateSpan.innerHTML = action === "open" ? "Open" : "Closed";
                        bridge.setAttribute("class", "bridge " + (action === "open" ? "open" : "closed"));
                    }, 3000);
                }
            } else {
                console.error(`Failed to send command: ${action}, status: ${res.status}`);
            }
        }).catch(err => console.error(`Error sending command: ${err}`));
    }

    function updateState() {
        fetch("/api/state")
            .then(res => {
                if (!res.ok) {
                    console.error(`Failed to fetch state, status: ${res.status}`);
                    return null;
                }
                return res.json();
            })
            .then(data => {
                if (data) {
                    console.log(`Received state: ${JSON.stringify(data)}`);
                    stateSpan.innerHTML = data.currentState || "Unknown";
                    bridgeStateSpan.innerHTML = data.bridgeState ? "Open" : "Closed";
                    bridge.setAttribute("class", "bridge " + (data.bridgeState ? "open" : "closed"));
                    // Update Traffic Lights
                    trafficRed.className = "light-status " + (data.redLedA ? "red" : "off");
                    trafficYellow.className = "light-status " + (data.yellowLedA ? "yellow" : "off");
                    trafficGreen.className = "light-status " + (data.greenLedA ? "green" : "off");
                    // Update Boat Lights
                    boatRed.className = "light-status " + (data.redLedB ? "red" : "off");
                    boatYellow.className = "light-status " + (data.yellowLedB ? "yellow" : "off");
                    boatGreen.className = "light-status " + (data.greenLedB ? "green" : "off");
                    console.log(`Updated lights: Traffic Red=${data.redLedA}, Yellow=${data.yellowLedA}, Green=${data.greenLedA}, Boat Red=${data.redLedB}, Yellow=${data.yellowLedB}, Green=${data.greenLedB}`);
                } else {
                    console.warn("No state data received");
                }
            })
            .catch(err => console.error(`Error fetching state: ${err}`));
    }

    setInterval(updateState, 200);
    updateState();
}