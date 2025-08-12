
// Tinkercad Test for H-Bridge and DC Motor (Simulates Lift and Lower)
// Wiring: H-Bridge (IN1=3, IN2=4, ENA=5, VCC=5V, GND), Motor to OUT1/OUT2

const int in1Pin = 3; // Direction: HIGH for lift (forward)
const int in2Pin = 4; // Direction: HIGH for lower (reverse)
const int enaPin = 5; // PWM for speed

void setup() {
  Serial.begin(9600);      // Start serial communication
  pinMode(in1Pin, OUTPUT);
  pinMode(in2Pin, OUTPUT);
  pinMode(enaPin, OUTPUT);
  Serial.println("H-Bridge Motor Test Started");
}

void loop() {
  // Lift (forward)
  digitalWrite(in1Pin, HIGH);
  digitalWrite(in2Pin, LOW);
  analogWrite(enaPin, 128); // Half speed
  Serial.println("Motor Lift (Opening Bridge)");
  delay(3000); // 3 seconds
  
  // Stop
  digitalWrite(in1Pin, LOW);
  digitalWrite(in2Pin, LOW);
  analogWrite(enaPin, 0);
  Serial.println("Motor Stop");
  delay(1000); // 1 second
  
  // Lower (reverse)
  digitalWrite(in1Pin, LOW);
  digitalWrite(in2Pin, HIGH);
  analogWrite(enaPin, 128); // Half speed
  Serial.println("Motor Lower (Closing Bridge)");
  delay(3000); // 3 seconds
  
  // Stop
  digitalWrite(in1Pin, LOW);
  digitalWrite(in2Pin, LOW);
  analogWrite(enaPin, 0);
  Serial.println("Motor Stop");
  delay(1000); // 1 second
}
