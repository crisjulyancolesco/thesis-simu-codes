// Include necessary libraries
#include <Arduino.h>

// Define pins for Ultrasonic Sensor 1 (Trigger and Echo)
#define TRIG_PIN1 5
#define ECHO_PIN1 18
#define LED_RED 2

// Define pins for Ultrasonic Sensor 2 (Trigger and Echo)
#define TRIG_PIN2 19
#define ECHO_PIN2 21
#define LED_YELLOW 4

// Define pins for Ultrasonic Sensor 3 (Trigger and Echo)
#define TRIG_PIN3 22
#define ECHO_PIN3 23
#define LED_GREEN 15

// Threshold distance in cm
#define DISTANCE_THRESHOLD 10

// Function to calculate distance using ultrasonic sensor
float getDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  float distance = (duration * 0.034) / 2; // Convert to cm
  return distance;
}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);

  // Initialize pins for Ultrasonic Sensor 1
  pinMode(TRIG_PIN1, OUTPUT);
  pinMode(ECHO_PIN1, INPUT);
  pinMode(LED_RED, OUTPUT);

  // Initialize pins for Ultrasonic Sensor 2
  pinMode(TRIG_PIN2, OUTPUT);
  pinMode(ECHO_PIN2, INPUT);
  pinMode(LED_YELLOW, OUTPUT);

  // Initialize pins for Ultrasonic Sensor 3
  pinMode(TRIG_PIN3, OUTPUT);
  pinMode(ECHO_PIN3, INPUT);
  pinMode(LED_GREEN, OUTPUT);
}

void loop() {
  // Get distance from Ultrasonic Sensor 1
  float distance1 = getDistance(TRIG_PIN1, ECHO_PIN1);
  Serial.print("Sensor 1 Distance: ");
  Serial.println(distance1);

  // Get distance from Ultrasonic Sensor 2
  float distance2 = getDistance(TRIG_PIN2, ECHO_PIN2);
  Serial.print("Sensor 2 Distance: ");
  Serial.println(distance2);

  // Get distance from Ultrasonic Sensor 3
  float distance3 = getDistance(TRIG_PIN3, ECHO_PIN3);
  Serial.print("Sensor 3 Distance: ");
  Serial.println(distance3);

  // Check distance and light up the corresponding LED
  if (distance1 < DISTANCE_THRESHOLD) {
    digitalWrite(LED_RED, HIGH);
  } else {
    digitalWrite(LED_RED, LOW);
  }

  if (distance2 < DISTANCE_THRESHOLD) {
    digitalWrite(LED_YELLOW, HIGH);
  } else {
    digitalWrite(LED_YELLOW, LOW);
  }

  if (distance3 < DISTANCE_THRESHOLD) {
    digitalWrite(LED_GREEN, HIGH);
  } else {
    digitalWrite(LED_GREEN, LOW);
  }

  // Small delay for stability
  delay(200);
}
