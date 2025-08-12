
// Tinkercad Test for Slide Switch (Simulates Bridge Position)
// Wiring: Slide Switch (Pin 2 to one pin, GND to the other)

const int switchPin = 2;  // Slide switch pin

void setup() {
  Serial.begin(9600);            // Start serial communication
  pinMode(switchPin, INPUT_PULLUP); // Set pin as input with internal pull-up
  Serial.println("Slide Switch Test Started");
}

void loop() {
  bool bridgeOpen = !digitalRead(switchPin); // ON (LOW with pull-up) = open, OFF (HIGH) = closed
  
  // Print to Serial Monitor
  Serial.print("Bridge Position: ");
  Serial.println(bridgeOpen ? "Open (200 mm)" : "Closed (0 mm)");
  
  delay(500); // Update every 0.5 seconds
}
