
const int motorPin = 3; // PWM pin for motor control

void setup() {
  Serial.begin(9600);      // Start serial communication
  pinMode(motorPin, OUTPUT); // Set motor pin as output
  Serial.println("DC Motor Test Started");
}

void loop() {
  // Activate motor (simulate lifting)
  analogWrite(motorPin, 255); // Half speed (PWM 0-255)
  Serial.println("Motor ON: Simulating Bridge Lifting");
  delay(3000); // Run for 3 seconds
  
  // Stop motor
  analogWrite(motorPin, 0);
  Serial.println("Motor OFF: Bridge Stopped");
  delay(3000); // Stop for 3 seconds
}