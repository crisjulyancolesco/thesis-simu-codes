#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <ESP32Servo.h>
#include <Adafruit_PN532.h>

//For HTTP Requests
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "OnePlus 10 Pro 5G";
const char* password = "A123456789";
unsigned long startAttemptTime = 0;
unsigned long connectionTimeout = 5000; // 30 seconds timeout for Wi-Fi connection

// Supabase API information
String API_URL = "https://bqpbvkwmgllnlufoszdd.supabase.co/rest/v1";
String API_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImJxcGJ2a3dtZ2xsbmx1Zm9zemRkIiwicm9sZSI6ImFub24iLCJpYXQiOjE3MzI5NDkwMTksImV4cCI6MjA0ODUyNTAxOX0.4iWAnKoyyC_3LH1GEgk4Kn-Ezi5ilG2aS49zfEgvAxw";
String AUTH_BEARER = "Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImJxcGJ2a3dtZ2xsbmx1Zm9zemRkIiwicm9sZSI6InNlcnZpY2Vfcm9sZSIsImlhdCI6MTczMjk0OTAxOSwiZXhwIjoyMDQ4NTI1MDE5fQ.jQrnieDZlwDwPNnT2yM3U3LzLmLHjSGTHNmjoH608fE";

// Id of the trash can, must be specified each time new trashcan is added.
String Trash1_red_id = "e5209cdb-7aa1-4cc0-990a-637eda6e1399"; // Replace with the actual ID
String Trash1_green_id = "e5209cdb-7aa1-4cc0-990a-637eda6e1399";
String Trash1_yellow_id = "e5209cdb-7aa1-4cc0-990a-637eda6e1399";

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

// Create a Servo object
Servo servo;

// Create the PN532 object
Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);

// Define the authorized UID (pass)
uint8_t authorizedUID[] = {0xFA, 0xD6, 0xAF, 0x80};
const uint8_t authorizedUIDLength = 4;

// Threshold distance in cm
#define DISTANCE_THRESHOLD 20

int fullCount = 0;

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

  // Wifi Connection
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  startAttemptTime = millis();
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    
    // Check if the connection attempt has timed out
    if (millis() - startAttemptTime >= connectionTimeout) {
      Serial.println("Failed to connect to Wi-Fi.");
      Serial.println("Restarting...");
      
      // Restart the ESP32
      ESP.restart();
    }
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

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
}

// Function to Fetch a specific table from trash_bins table from DB
String fetchHttpResponse(const String& filterKey, const String& filterValue, const String& column, const String& table) {
  HTTPClient http;

  // Construct API URL with query parameters for filtering and selecting a specific column
  String queryUrl = API_URL + "/" + table + "?select=" + column + "&" + filterKey + "=eq." + filterValue;

  http.begin(queryUrl);
  http.addHeader("apikey", API_KEY);
  http.addHeader("Authorization", AUTH_BEARER);

  int httpCode = http.GET(); // Send GET request
  if (httpCode == 200) {
    String payload = http.getString();
    http.end(); // Close connection
    return payload;
  } else {
    Serial.print("Error in HTTP request: ");
    Serial.println(httpCode);
    http.end();
    return "";
  }
}

// Function to PATCH the fullness level to the DB
bool updateToServer(const String& id, const String& column, int value, const String& table) {
  HTTPClient http;

  // Construct the query URL with the ID
  String queryUrl = API_URL + "/" + table + "?id=eq." + id;

  // Initialize the HTTP request
  http.begin(queryUrl);
  http.addHeader("apikey", API_KEY);
  http.addHeader("Authorization", AUTH_BEARER);

  // Construct JSON payload dynamically based on column and value
  String jsonPayload = "{";
  jsonPayload += "\"" + column + "\": " + String(value);
  jsonPayload += "}";

  // Send the PATCH request to update the specified column
  int httpCode = http.PATCH(jsonPayload);  // Send PATCH request
  if (httpCode == 200 || httpCode == 204) {
    Serial.println("Update successful");
    http.end();
    return true;
  } else {
    Serial.print("Error in HTTP request: ");
    Serial.println(httpCode);
    http.end();
    return false;
  }
}

// Function to destructure results from requests
DynamicJsonDocument processResponse(const String& payload) {
  DynamicJsonDocument doc(1024);
  if (payload.isEmpty()) {
    Serial.println("Empty payload received.");
    return doc; // Return empty document
  }

  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.println("Failed to parse JSON");
  }

  return doc;
}

bool detectingFullness(const String& id) {
  if (WiFi.status() == WL_CONNECTED) {
    // Specify the ID and column to fetch
    String column = "isLocked";

    String response = fetchHttpResponse("id",id, column, "trash_bins");  // Assuming this fetches the HTTP response correctly
    DynamicJsonDocument doc = processResponse(response);  // Assuming this processes the response correctly

    // Example: Extract and use data from the response
    if (!doc.isNull() && doc.size() > 0 && doc[0].containsKey(column)) {
      bool lockStatus = doc[0][column];  // Now lockStatus is boolean
      Serial.print("Locked Status: ");
      Serial.println(lockStatus ? "true" : "false");
      return !lockStatus;  // Return boolean value
    } else {
      Serial.println("Error: Column not found or invalid response");
    }
  } else {
    Serial.println("Error in WiFi connection");
  }

  return false;  // Default return value when WiFi is not connected or no valid response
}

// Function to find the RFID in the database
bool checkRFID(const String& rfid) {
  if (WiFi.status() == WL_CONNECTED) {
    // Specify the column and table
    String column = "RFID";

    String response = fetchHttpResponse("RFID",rfid, column, "users_details");  // Assuming this fetches the HTTP response correctly
    DynamicJsonDocument doc = processResponse(response);  // Assuming this processes the response correctly

    // Check if the response contains the RFID value
    if (!doc.isNull() && doc.size() > 0 && doc[0].containsKey(column)) {
      String dbRFID = doc[0][column];  // Extract RFID from the database
      Serial.print("Database RFID: ");
      Serial.println(dbRFID);

      return true;
    } else {
      Serial.println("Error: Column not found or invalid response.");
    }
  } else {
    Serial.println("Error in WiFi connection.");
  }

  return false;  // Default return value when WiFi is not connected or no valid response
}

void loop() {
  if (detectingFullness(Trash1_red_id)) {
    // Get the distance
    float distance = getDistance();
    float percentage;

    if (distance > 60) {
      percentage = 0;
    } else if (distance < 20) {
      percentage = 100;
      fullCount++;
      if (fullCount >= 5) {
        updateToServer(Trash1_red_id, "isLocked", true, "trash_bins");
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
    } else {
      lcd.print("                   ");
      servo.write(145); // Unlock the bin
    }

    // Print to Serial Monitor for debugging
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");

    updateToServer(Trash1_red_id, "fullness_level", percentage, "trash_bins");

  } else {
    lcd.setCursor(0, 2);
    lcd.print("LOCKED: NFC REQ.");
    servo.write(35); // Lock the bin
    // Detect NFC tag
    uint8_t success;
    uint8_t uid[7];
    uint8_t uidLength;

    success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
    char uidString[50];

    if (success) {
      Serial.println("NFC tag detected!");

      // Clear the string
      uidString[0] = '\0';

      // Convert the UID to a string
      for (uint8_t i = 0; i < uidLength; i++) {
          char buffer[6]; // Temporary buffer to hold each byte in "0xHH " format
          sprintf(buffer, "0x%02X", uid[i]);
          strcat(uidString, buffer); // Append to the uidString
      }
      Serial.print("UID: ");
      Serial.println(uidString); // Print the UID as a single string

      // Check if the detected UID matches the authorized UID
      if (uidLength == authorizedUIDLength && checkRFID(uidString)) {
        Serial.println("Authorized UID! Resuming detection.");
        lcd.setCursor(0, 2);
        lcd.clear();
        lcd.print("UNLOCKED");
        updateToServer(Trash1_red_id, "isLocked", false, "trash_bins");
        fullCount = 0; // Reset full count
        delay(500);
      } else {
        Serial.println("Unauthorized UID.");
        lcd.setCursor(0, 2);
        lcd.clear();
        lcd.print("Unauthorized UID.");
      }
    }
  }

  // Small delay for stability
  delay(3000);
}
