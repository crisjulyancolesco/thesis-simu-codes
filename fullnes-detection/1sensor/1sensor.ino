#define TRIG_PIN 4
#define ECHO_PIN 15
#define RELAY_PIN 21 // Relay control pin

void setup() {
  Serial.begin(115200);

  // Ultrasonic sensor pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Relay pin
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Initially, solenoid is locked (Red LED ON)
}

void loop() {
  // Measure distance
  long duration = getUltrasonicDistance();
  float distance = duration * 0.034 / 2; // Convert to cm

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  // Toggle relay and LEDs based on distance
  if (distance < 10.0) {
    // Activate relay (unlock solenoid, Green LED ON)
    digitalWrite(RELAY_PIN, LOW); // Switch to NO (Green LED)
    Serial.println("Solenoid Lock Activated! (RED LED ON)");
  } else {
    // Deactivate relay (lock solenoid, Red LED ON)
    digitalWrite(RELAY_PIN, HIGH); // Switch to NC (Red LED)
    Serial.println("Solenoid Lock Deactivated! (GREEN LED ON)");
  }

  delay(500); // Delay for stability
}

// Function to get the distance from the ultrasonic sensor
long getUltrasonicDistance() {
  // Trigger a pulse
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Measure the duration of echo
  return pulseIn(ECHO_PIN, HIGH);
}
