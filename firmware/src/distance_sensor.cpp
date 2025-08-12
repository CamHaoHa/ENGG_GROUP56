// Simple Arduino Code to Test HC-SR04 Ultrasonic Sensor in Tinkercad
// Wiring: Trig to Pin 9, Echo to Pin 10, VCC to 5V, GND to GND
// This code reads the distance and prints it to the Serial Monitor
// Use the Serial Monitor to see output; adjust the distance slider in Tinkercad to test
const int trigPin = 9;  // Trigger pin
const int echoPin = 10; // Echo pin
void setup() {
Serial.begin(9600);      // Start serial communication
pinMode(trigPin, OUTPUT); // Set trig as output
pinMode(echoPin, INPUT);  // Set echo as input
Serial.println("HC-SR04 Sensor Test Started");
}
void loop() {
// Trigger the sensor
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    // Read the echo duration
    long duration = pulseIn(echoPin, HIGH);
    // Calculate distance in cm
    float distance = duration * 0.034 / 2;
    // Print to Serial Monitor
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");
    if(distance < 50) {
        Serial.println("Ship detected!");
    } 
dela
    delay(500); // Wait 0.5 seconds before next reading
}

//convert the echo duration to distance in centimeters using the formula   float distance = duration * 0.034 / 2; (speed of sound is 34cm/s , divided by 2 for ground trip travel)