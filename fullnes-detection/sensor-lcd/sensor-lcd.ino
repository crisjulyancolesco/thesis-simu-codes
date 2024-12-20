#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Define pins for Ultrasonic Sensor
#define TRIG_PIN 5
#define ECHO_PIN 18

// Initialize the 20x4 I2C LCD
LiquidCrystal_I2C lcd(0x27, 20, 4); // Address 0x27 is typical for most LCDs

// Threshold distance in cm
#define DISTANCE_THRESHOLD 10

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
}

void loop() {
  // Get the distance
  float distance = getDistance();
  
  // Display the distance on the LCD
  lcd.setCursor(0, 1); // Line 2
  lcd.print("Distance: ");
  lcd.print(distance);
  lcd.print(" cm     "); // Padding to clear old values
  
  // Check if the distance is less than the threshold
  lcd.setCursor(0, 2); // Line 3
  if (distance < DISTANCE_THRESHOLD) {
    lcd.print("FULL               "); // Clear old values with spaces
  } else {
    lcd.print("                   "); // Clear the line if not full
  }
  
  // Print to Serial Monitor for debugging
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  // Small delay for stability
  delay(500);
}
