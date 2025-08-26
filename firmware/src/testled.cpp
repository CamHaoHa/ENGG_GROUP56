// Control LEDs on pins D15, D2, and D4

int ledPins[] = {15, 2, 4}; // Pins with LEDs

void setup() {
  // Set all pins as outputs
  for (int i = 0; i < 3; i++) {
    pinMode(ledPins[i], OUTPUT);
  }
}

void loop() {
  // Turn all LEDs ON
  for (int i = 0; i < 3; i++) {
    digitalWrite(ledPins[i], HIGH);
  }
  delay(1000); // Wait 1 second

  // Turn all LEDs OFF
  for (int i = 0; i < 3; i++) {
    digitalWrite(ledPins[i], LOW);
  }
  delay(1000); // Wait 1 second
}
