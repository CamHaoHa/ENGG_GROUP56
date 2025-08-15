// Tinkercad Simulation for Vertical Lift Bridge (HC-SR04 + Slide Switch + DC Motor)
// Tests ship detection (HC-SR04), bridge position (slide switch), and motor control (H-Bridge)
// Wiring:
// - HC-SR04: Trig=9, Echo=10, VCC=5V, GND
// - Slide Switch: Pin 2 to one pin, GND to the other (ON = open, OFF = closed)
// - H-Bridge/DC Motor: IN1=6 (forward/lift), IN2=7 (reverse/lower), ENA=3 (PWM speed), VCC=5V, GND, OUT1/OUT2 to motor

enum State {
  READY,
  SHIP_DETECTED,
  TRAFFIC_STOPPED,
  BRIDGE_OPENING,
  BRIDGE_OPEN,
  SHIP_PASSING,
  SHIP_PASSED,
  BRIDGE_CLOSING,
  ERROR,
  OVERRIDE
};

// Pins
const int trigPin = 9;    // HC-SR04 trigger
const int echoPin = 10;   // HC-SR04 echo
const int switchPin = 2;  // Slide switch for bridge position
const int in1Pin = 6;     // Motor direction: HIGH for lift (forward)
const int in2Pin = 7;     // Motor direction: HIGH for lower (reverse)
const int enaPin = 3;     // PWM for motor speed

// Variables
State currentState = READY;
bool shipDetected = false;
bool bridgeOpen = false;  // Based on slide switch: true = open (200 mm), false = closed (0 mm)
float motorCurrent = 0;   // Simulated current in Amps (for 5A limit)

void setup() {
  Serial.begin(9600);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(switchPin, INPUT_PULLUP); // Internal pull-up for switch
  pinMode(in1Pin, OUTPUT);
  pinMode(in2Pin, OUTPUT);
  pinMode(enaPin, OUTPUT);
  
  // Initial state
  digitalWrite(in1Pin, LOW);
  digitalWrite(in2Pin, LOW);
  analogWrite(enaPin, 0);
  Serial.println("System Ready, Bridge Closed");
}

void loop() {
  // Read sensors
  shipDetected = readProximitySensor();
  bridgeOpen = !digitalRead(switchPin); // ON (LOW) = open, OFF (HIGH) = closed
  
  // Simulate override via Serial input (e.g., 'o' for open, 'c' for close)
  if (Serial.available() > 0) {
    char command = Serial.read();
    handleOverride(command);
  }
  
  // FSM
  switch (currentState) {
    case READY:
      if (shipDetected) {
        currentState = SHIP_DETECTED;
        Serial.println("Ship Detected");
      }
      break;
    case SHIP_DETECTED:
      currentState = TRAFFIC_STOPPED;
      Serial.println("Traffic Stopped");
      break;
    case TRAFFIC_STOPPED:
      currentState = BRIDGE_OPENING;
      motorCurrent = 3.0; // Simulated draw
      liftBridge(true); // Start lifting
      Serial.println("Bridge Opening");
      break;
    case BRIDGE_OPENING:
      if (motorCurrent > 4.5) {
        currentState = ERROR;
        stopMotor();
        motorCurrent = 0;
        Serial.println("Error: Motor Overload");
        break;
      }
      if (bridgeOpen) { // Switch toggled to ON
        currentState = BRIDGE_OPEN;
        stopMotor();
        Serial.println("Bridge Open");
      }
      break;
    case BRIDGE_OPEN:
      currentState = SHIP_PASSING;
      Serial.println("Ship Passing");
      break;
    case SHIP_PASSING:
      if (!shipDetected) {
        currentState = SHIP_PASSED;
        Serial.println("Ship Passed");
      }
      break;
    case SHIP_PASSED:
      currentState = BRIDGE_CLOSING;
      motorCurrent = 3.0;
      liftBridge(false); // Start lowering
      Serial.println("Bridge Closing");
      break;
    case BRIDGE_CLOSING:
      if (motorCurrent > 4.5) {
        currentState = ERROR;
        stopMotor();
        motorCurrent = 0;
        Serial.println("Error: Motor Overload");
        break;
      }
      if (!bridgeOpen) { // Switch toggled to OFF
        currentState = READY;
        stopMotor();
        Serial.println("Bridge Closed");
      }
      break;
    case ERROR:
      // Wait for reset
      break;
    case OVERRIDE:
      // Handled in handleOverride
      break;
  }
  
  // Update Serial Monitor (mock UI)
  Serial.print("State: ");
  Serial.print(getStateString());
  Serial.print(", Ship Detected: ");
  Serial.print(shipDetected ? "Yes" : "No");
  Serial.print(", Bridge Position: ");
  Serial.print(bridgeOpen ? "Open (200 mm)" : "Closed (0 mm)");
  Serial.print(", Motor Current: ");
  Serial.print(motorCurrent);
  Serial.println(" A");
  
  delay(1000); // Update every 1 second
}

// Get state string
String getStateString() {
  switch (currentState) {
    case READY: return "System Ready";
    case SHIP_DETECTED: return "Ship Detected";
    case TRAFFIC_STOPPED: return "Traffic Stopped";
    case BRIDGE_OPENING: return "Bridge Opening";
    case BRIDGE_OPEN: return "Bridge Open";
    case SHIP_PASSING: return "Ship Passing";
    case SHIP_PASSED: return "Ship Passed";
    case BRIDGE_CLOSING: return "Bridge Closing";
    case ERROR: return "Error";
    case OVERRIDE: return "Override";
    default: return "Unknown";
  }
}

// Read proximity sensor
bool readProximitySensor() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH);
  float distance = duration * 0.034 / 2; // cm
  return distance < 50; // Ship detected if <50 cm
}

// Lift/lower bridge (true = lift/forward, false = lower/reverse)
void liftBridge(bool lift) {
  if (lift) {
    digitalWrite(in1Pin, HIGH);
    digitalWrite(in2Pin, LOW);
  } else {
    digitalWrite(in1Pin, LOW);
    digitalWrite(in2Pin, HIGH);
  }
  analogWrite(enaPin, 128); // Half speed
}

// Stop motor
void stopMotor() {
  digitalWrite(in1Pin, LOW);
  digitalWrite(in2Pin, LOW);
  analogWrite(enaPin, 0);
}

// Handle override
void handleOverride(char command) {
  currentState = OVERRIDE;
  switch (command) {
    case 'o': // Open bridge
      liftBridge(true);
      Serial.println("Override: Bridge Opening");
      // In real setup, wait for position; here assume manual switch toggle
      break;
    case 'c': // Close bridge
      liftBridge(false);
      Serial.println("Override: Bridge Closing");
      break;
    case 'e': // Reset error
      currentState = READY;
      stopMotor();
      motorCurrent = 0;
      Serial.println("Override: Error Reset");
      break;
  }
}