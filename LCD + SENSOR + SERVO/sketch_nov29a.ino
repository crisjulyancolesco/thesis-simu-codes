#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <ESP32Servo.h> // Use ESP32Servo library

// Define pins for Ultrasonic Sensor
#define TRIG_PIN 5
#define ECHO_PIN 18

// Initialize the 20x4 I2C LCD
LiquidCrystal_I2C lcd(0x27, 20, 4); // Address 0x27 is typical for most LCDs

// Define pin for Servo Motor
#define SERVO_PIN 27 // GPIO 27 for Servo on ESP32

// Threshold distance in cm
#define DISTANCE_THRESHOLD 10

// Create a Servo object
Servo servo;

// Function to calculate distance using ultrasonic sensor
float getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  float distance = (duration * 0.034) / 2; // Convert to cm
  return distance;
}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);

  // Initialize pins for Ultrasonic Sensor
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Initialize the LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ultrasonic Sensor");

  // Attach the Servo to the specified pin and set initial position
  servo.attach(SERVO_PIN);
  servo.write(90); // Start at 90 degrees (neutral position)
}

void loop() {
  // Get the distance
  float distance = getDistance();

  float percentage;

  if (distance > 60) {
      percentage = 0;
  } else if (distance < 20) {
      percentage = 100;
  } else if (distance <= 60 && distance >= 20) {
      percentage = (1 - ((distance - 20) / 40)) * 100;
  }

  // Display the fullness level on the LCD
  lcd.setCursor(0, 0); // Line 1
  lcd.print("Fullness Level: ");
  lcd.setCursor(4, 1); // Line 2
  lcd.print(percentage);
  lcd.print(" %     ");

  // Check if the distance is less than the threshold and control the servo
  lcd.setCursor(0, 2); // Line 3
  if (distance < DISTANCE_THRESHOLD) {
    lcd.print("FULL               "); // Clear old values with spaces
    servo.write(35); // Rotate servo to 0 degrees (left)
  } else {
    lcd.print("                   "); // Clear the line if not full
    servo.write(145); // Rotate servo back to 90 degrees (neutral)
  }

  // Print to Serial Monitor for debugging
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  // Small delay for stability
  delay(500);
}
