#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

// LCD Addresses (ensure these match your devices)
LiquidCrystal_I2C lcd1(0x27, 16, 2); // Address 0x27
LiquidCrystal_I2C lcd2(0x26, 16, 2); // Address 0x26
LiquidCrystal_I2C lcd3(0x25, 16, 2); // Address 0x25

// Ultrasonic Sensor Pins
#define TRIG1 5
#define ECHO1 18
#define TRIG2 4
#define ECHO2 19
#define TRIG3 13
#define ECHO3 14

// Servo Motor Pin
#define SERVO_PIN 27
Servo myServo;

void setup() {
  Serial.begin(115200);

  // Initialize LCDs
  lcd1.init();
  lcd2.init();
  lcd3.init();
  lcd1.backlight();
  lcd2.backlight();
  lcd3.backlight();

  lcd1.print("LCD1: OK");
  lcd2.print("LCD2: OK");
  lcd3.print("LCD3: OK");

  // Initialize Ultrasonic Sensor Pins
  pinMode(TRIG1, OUTPUT);
  pinMode(ECHO1, INPUT);
  pinMode(TRIG2, OUTPUT);
  pinMode(ECHO2, INPUT);
  pinMode(TRIG3, OUTPUT);
  pinMode(ECHO3, INPUT);

  // Initialize Servo Motor
  myServo.attach(SERVO_PIN);
  myServo.write(0); // Start at 0 degrees
  delay(2000);      // Allow initialization
}

void loop() {
  // Measure distance for each sensor
  float distance1 = measureDistance(TRIG1, ECHO1);
  delay(1000);
  float distance2 = measureDistance(TRIG2, ECHO2);
  delay(1000);
  float distance3 = measureDistance(TRIG3, ECHO3);
  delay(1000);

  // Print distances to Serial Monitor
  Serial.print("Sensor 1 Distance: ");
  Serial.print(distance1);
  Serial.println(" cm");

  Serial.print("Sensor 2 Distance: ");
  Serial.print(distance2);
  Serial.println(" cm");

  Serial.print("Sensor 3 Distance: ");
  Serial.print(distance3);
  Serial.println(" cm");

  // Display distances on respective LCDs
  lcd1.setCursor(0, 1);
  lcd1.print("Dist: ");
  lcd1.print(distance1);
  lcd1.print(" cm  ");

  lcd2.setCursor(0, 1);
  lcd2.print("Dist: ");
  lcd2.print(distance2);
  lcd2.print(" cm  ");

  lcd3.setCursor(0, 1);
  lcd3.print("Dist: ");
  lcd3.print(distance3);
  lcd3.print(" cm  ");

  // Test Servo Motor
  // for (int angle = 0; angle <= 180; angle += 30) {
  //   myServo.write(angle);
  //   delay(500);
  // }
  // for (int angle = 180; angle >= 0; angle -= 30) {
  //   myServo.write(angle);
  //   delay(500);
  // }

  delay(2000); // Wait before the next cycle
}

// Function to measure distance using an ultrasonic sensor
float measureDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  float distance = (duration / 2.0) * 0.0343; // Convert to cm
  return distance;
}
