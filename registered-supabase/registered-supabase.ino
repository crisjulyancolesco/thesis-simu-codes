#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <ESP32Servo.h>
#include <Adafruit_PN532.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h> // Library for parsing JSON

// Define pins for Ultrasonic Sensor
#define TRIG_PIN 5
#define ECHO_PIN 18

// Initialize the 20x4 I2C LCD
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Define pin for Servo Motor
#define SERVO_PIN 27

// Define I2C pins for ESP32
#define SDA_PIN 21
#define SCL_PIN 22

// Define LED pin
#define LED_PIN 2

// Create a Servo object
Servo servo;

// Create the PN532 object
Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);

// Define the WiFi credentials
const char* ssid = "Redmi Note 10 Pro";        // Replace with your WiFi SSID
const char* password = "jb123456"; // Replace with your WiFi password

// Define the Supabase URL and API key
const String supabaseUrl = "https://bqpbvkwmgllnlufoszdd.supabase.co/rest/v1/trash_bins";
const String supabaseApiKey = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImJxcGJ2a3dtZ2xsbmx1Zm9zemRkIiwicm9sZSI6ImFub24iLCJpYXQiOjE3MzI5NDkwMTksImV4cCI6MjA0ODUyNTAxOX0.4iWAnKoyyC_3LH1GEgk4Kn-Ezi5ilG2aS49zfEgvAxw"; // Replace with your Supabase API key

// Variable to hold the authorized UID fetched from Supabase
uint8_t authorizedUID[4]; // We expect the UID to be 4 bytes
const uint8_t authorizedUIDLength = 4;

// Threshold distance in cm
#define DISTANCE_THRESHOLD 20

int fullCount = 0;
bool detectingFullness = true;
bool unlocked = false; // Flag to check if the system is unlocked

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

// Function to compare two UIDs
bool compareUIDs(uint8_t *uid1, uint8_t *uid2, uint8_t length) {
  for (uint8_t i = 0; i < length; i++) {
    if (uid1[i] != uid2[i]) {
      Serial.print("UID mismatch at byte ");
      Serial.println(i);
      Serial.print("UID1: 0x");
      Serial.print(uid1[i], HEX);
      Serial.print(" UID2: 0x");
      Serial.println(uid2[i], HEX);
      return false;
    }
  }
  return true;
}

// Function to fetch the authorized UID from Supabase
// Function to fetch the authorized UID from Supabase
void fetchAuthorizedUID() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Create an HTTP client to send requests
  HTTPClient http;
  http.begin(supabaseUrl);
  http.addHeader("Authorization", "Bearer " + supabaseApiKey);
  http.addHeader("apikey", supabaseApiKey);
  
  int httpCode = http.GET();
  if (httpCode == 200) {
    String response = http.getString();
    Serial.println("Response: " + response);
    
    // Parse the JSON response using ArduinoJson library
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, response);
    
    // Assuming the response contains an array and we are fetching the first entry's registered field
    String uidHex = doc[0]["registered"]; // Get the registered UID as a hex string
    
    if (uidHex.length() > 0) { // If there is a valid UID in the registered field
      // Split the string by spaces
      int start = 0;
      for (int i = 0; i < authorizedUIDLength; i++) {
        int end = uidHex.indexOf(' ', start);
        if (end == -1) {
          end = uidHex.length();
        }
        
        String hexByte = uidHex.substring(start, end);
        hexByte.replace("0x", ""); // Remove the "0x" prefix
        
        // Convert the hex byte to an integer and store it in the array
        authorizedUID[i] = strtol(hexByte.c_str(), NULL, 16);
        
        start = end + 1; // Move to the next byte
      }

      // Set unlocked flag to true if UID exists
      unlocked = true;
      Serial.println("Authorized UID fetched:");
      for (int i = 0; i < authorizedUIDLength; i++) {
        Serial.print("0x");
        Serial.print(authorizedUID[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
    } else {
      Serial.println("No valid UID found in registered field.");
    }
  } else {
    Serial.println("HTTP request failed. Status: " + String(httpCode));
  }
  http.end();
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
  servo.write(90); // Neutral position

  // Initialize the LED pin as OUTPUT
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Initialize PN532
  Wire.begin(SDA_PIN, SCL_PIN);
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("PN532 not found!");
    while (1);
  }
  nfc.SAMConfig(); // Configure PN532 to read NFC tags
  Serial.println("Waiting for an NFC tag...");

  // Fetch authorized UID from Supabase
  fetchAuthorizedUID();
}

void loop() {
  if (unlocked) {
    if (detectingFullness) {
      // Get the distance
      float distance = getDistance();
      float percentage;

      if (distance > 60) {
        percentage = 0;
      } else if (distance < 20) {
        percentage = 100;
        fullCount++;
        if (fullCount >= 5) {
          detectingFullness = false;
        }
      } else if (distance <= 60 && distance >= 20) {
        percentage = (1 - ((distance - 20) / 40)) * 100;
      }

      // Display the fullness level on the LCD
      lcd.setCursor(0, 0);
      lcd.print("Fullness Level: ");
      lcd.setCursor(4, 1);
      lcd.print(percentage);
      lcd.print(" %     ");

      // Check if the distance is less than the threshold and control the servo
      lcd.setCursor(0, 2);
      if (distance < DISTANCE_THRESHOLD) {
        lcd.print("FULL               ");
        servo.write(35); // Lock the bin
      } else {
        lcd.print("                   ");
        servo.write(145); // Unlock the bin
      }

      // Print to Serial Monitor for debugging
      Serial.print("Distance: ");
      Serial.print(distance);
      Serial.println(" cm");

    } else {
      lcd.setCursor(0, 2);
      lcd.print("LOCKED: NFC REQ.");
      // Detect NFC tag
      uint8_t success;
      uint8_t uid[7];
      uint8_t uidLength;

      success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

      if (success) {
        Serial.println("NFC tag detected!");
        Serial.print("UID: ");
        for (uint8_t i = 0; i < uidLength; i++) {
          Serial.print("0x");
          Serial.print(uid[i], HEX);
          if (i < uidLength - 1) Serial.print(" ");
        }
        Serial.println();

        // Check if the detected UID matches the authorized UID
        if (uidLength == authorizedUIDLength && compareUIDs(uid, authorizedUID, uidLength)) {
          Serial.println("Authorized UID! Resuming detection.");
          lcd.setCursor(0, 2);
          lcd.clear();
          lcd.print("UNLOCKED");
          detectingFullness = true;
          fullCount = 0; // Reset full count
          digitalWrite(LED_PIN, HIGH); // Turn LED ON
          delay(500);
          digitalWrite(LED_PIN, LOW); // Turn LED OFF
        } else {
          Serial.println("Unauthorized UID.");
          lcd.setCursor(0, 2);
          lcd.clear();
          lcd.print("Unauthorized UID.");
        }
      }
    }
  } else {
    lcd.setCursor(0, 2);
    lcd.print("Fetching UID...");
  }
}
