// Simulated system state and sensors
let bridgeOpen = false;
let vehicleSensor = false;
let boatSensor = false;
let overrides = { vehicle: false, boat: false };

function updateUI() {
  document.getElementById("bridgeStatus").textContent = bridgeOpen ? "Open" : "Closed";
  document.getElementById("vehicleSensor").textContent = vehicleSensor ? "Active" : "Inactive";
  document.getElementById("boatSensor").textContent = boatSensor ? "Active" : "Inactive";
  document.getElementById("systemState").textContent = bridgeOpen ? "Bridge is Open" : "Idle";

  // Update override buttons
  document.getElementById("vehicleOverrideBtn").textContent = 
    overrides.vehicle ? "Disable Vehicle Override" : "Enable Vehicle Override";
  document.getElementById("boatOverrideBtn").textContent = 
    overrides.boat ? "Disable Boat Override" : "Enable Boat Override";
}

function openBridge() {
  bridgeOpen = true;
  updateUI();
  sendCommand("open");
}

function closeBridge() {
  bridgeOpen = false;
  updateUI();
  sendCommand("close");
}

function toggleOverride(sensor) {
  overrides[sensor] = !overrides[sensor];
  console.log(`Override for ${sensor} sensor: ${overrides[sensor]}`);
  updateUI();
}

function sendCommand(command) {
  // Simulated real-time wireless communication
  console.log(`Sending command to bridge: ${command}`);
  // In a real setup, you’d use WebSockets or fetch() here.
}

// Simulated real-time sensor updates
setInterval(() => {
  if (!overrides.vehicle) vehicleSensor = Math.random() > 0.5;
  if (!overrides.boat) boatSensor = Math.random() > 0.5;
  updateUI();
}, 3000);

updateUI();
