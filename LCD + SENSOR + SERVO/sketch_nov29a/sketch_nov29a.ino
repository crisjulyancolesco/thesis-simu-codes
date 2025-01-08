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
String Trash1_green_id = "d87cf1cf-8fdc-452e-b390-0d4006efb686";
String Trash1_yellow_id = "6a3077ee-0d78-4c7c-8fd9-a8707a1a7778";

// Ultrasonic Sensor Pins
#define TRIG1 5
#define ECHO1 18
#define TRIG2 4
#define ECHO2 19
#define TRIG3 13
#define ECHO3 14

// Initialize the 20x4 I2C LCD
LiquidCrystal_I2C lcd1(0x27, 16, 2); // Address 0x27
LiquidCrystal_I2C lcd2(0x26, 16, 2); // Address 0x26
LiquidCrystal_I2C lcd3(0x25, 16, 2); // Address 0x25

// Define pin for Servo Motor
#define SERVO1_PIN 27
#define SERVO2_PIN 32
#define SERVO3_PIN 33

// Define I2C pins for ESP32
#define SDA_PIN 21
#define SCL_PIN 22

// Create a Servo object
Servo servo1;
Servo servo2;
Servo servo3;

// Create the PN532 object
Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);

// Define the authorized UID (pass)
const uint8_t authorizedUIDLength = 4;

// Threshold distance in cm
#define DISTANCE_THRESHOLD 20

// Global variables
int upperLimit;
int lowerLimit;

// Function to calculate distance using ultrasonic sensor
float getDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  float distance = (duration / 2.0) * 0.0343; // Convert to cm
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

  // Initialize Ultrasonic Sensor Pins
  pinMode(TRIG1, OUTPUT);
  pinMode(ECHO1, INPUT);
  pinMode(TRIG2, OUTPUT);
  pinMode(ECHO2, INPUT);
  pinMode(TRIG3, OUTPUT);
  pinMode(ECHO3, INPUT);

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

  // Attach the Servo to the specified pin and set initial position
  servo1.attach(SERVO1_PIN);
  servo2.attach(SERVO2_PIN);
  servo3.attach(SERVO3_PIN);

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
    // Specify the ID and columns to fetch
    String columns = "isLocked,upper_limit,lower_limit";

    // Fetch the HTTP response for the specified ID and columns
    String response = fetchHttpResponse("id", id, columns, "trash_bins");  // Assuming this fetches the HTTP response correctly
    DynamicJsonDocument doc = processResponse(response);  // Assuming this processes the response correctly

    // Extract and use data from the response
    if (!doc.isNull() && doc.size() > 0) {
      if (doc[0].containsKey("isLocked") && doc[0].containsKey("upper_limit") && doc[0].containsKey("lower_limit")) {
        // Update global variables
        bool lockStatus = doc[0]["isLocked"]; 
        upperLimit = doc[0]["upper_limit"];  // Assign integer value
        lowerLimit = doc[0]["lower_limit"];  // Assign integer value

        // Debugging output
        Serial.print("Locked Status: ");
        Serial.println(lockStatus ? "true" : "false");
        Serial.print("Upper Limit: ");
        Serial.println(upperLimit);
        Serial.print("Lower Limit: ");
        Serial.println(lowerLimit);

        // Return inverse of isLocked
        return !lockStatus;
      } else {
        Serial.println("Error: Columns not found or invalid response");
      }
    } else {
      Serial.println("Error: Invalid JSON response");
    }
  } else {
    Serial.println("Error in WiFi connection");
  }
  return true;  // Default return value in case of error
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

// Global variables for full counts
int fullCount1 = 0;
int fullCount2 = 0;
int fullCount3 = 0;

bool isLocked1 = false;
bool isLocked2 = false;
bool isLocked3 = false;

void loop() {
  if (isLocked1 && isLocked2 && isLocked3) {
    Serial.println("All bins are locked. Stopping detection.");
    handleNFC();
  } else {
    Serial.print("Red Trash");
    checkTrashBin(Trash1_red_id, TRIG1, ECHO1, lcd1, servo1, fullCount1, isLocked1);
    Serial.print("Green Trash");
    checkTrashBin(Trash1_green_id, TRIG2, ECHO2, lcd2, servo2, fullCount2, isLocked2);
    Serial.print("Yellow Trash");
    checkTrashBin(Trash1_yellow_id, TRIG3, ECHO3, lcd3, servo3, fullCount3, isLocked3);
    handleNFC();
  }
}

void checkTrashBin(String trashId, int trigPin, int echoPin, LiquidCrystal_I2C lcd, Servo servo, int &fullCount, bool &isLocked) {
  if (detectingFullness(trashId)) {
    isLocked = false;

    servo.write(180);
    delay(1000);

    // Get the distance
    float distance = getDistance(trigPin, echoPin);
    float percentage;

    if (distance > upperLimit) {
      percentage = 0;
      fullCount = 0;
    } else if (distance < lowerLimit) {
      percentage = 100;
      fullCount++;
      if (fullCount >= 5) {
        updateToServer(trashId, "isLocked", true, "trash_bins");
      }
    } else {
      percentage = (1 - ((distance - lowerLimit) / (upperLimit - lowerLimit))) * 100;
      fullCount = 0;
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
    }

    // Print to Serial Monitor for debugging
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");

    updateToServer(trashId, "fullness_level", percentage, "trash_bins");

  } else {
    isLocked = true;
    lcd.setCursor(0, 2);
    lcd.print("LOCKED: NFC REQ.");
    servo.write(0); // Lock the bin
    delay(1000);
    handleNFC();
    
  }
  delay(500); //Change for the delay in each bin
  return;
}

void handleNFC() {
  uint8_t success;
  uint8_t uid[7];
  uint8_t uidLength;

  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 3000);
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
      lcd1.setCursor(0, 2);
      lcd1.clear();
      lcd1.print("UNLOCKED");
      lcd2.setCursor(0, 2);
      lcd2.clear();
      lcd2.print("UNLOCKED");
      lcd3.setCursor(0, 2);
      lcd3.clear();
      lcd3.print("UNLOCKED");
      
      updateToServer(Trash1_red_id, "isLocked", false, "trash_bins");
      updateToServer(Trash1_yellow_id, "isLocked", false, "trash_bins");
      updateToServer(Trash1_green_id, "isLocked", false, "trash_bins");

      fullCount1 = 0;
      fullCount2 = 0;
      fullCount3 = 0;

      isLocked1 = 0;
      isLocked2 = 0;
      isLocked3 = 0;

      delay(1000);

      delay(500);
    } else {
      Serial.println("Unauthorized UID.");
      lcd1.setCursor(0, 2);
      lcd1.clear();
      lcd1.print("Unauthorized UID.");
      lcd2.setCursor(0, 2);
      lcd2.clear();
      lcd2.print("Unauthorized UID.");
      lcd3.setCursor(0, 2);
      lcd3.clear();
      lcd3.print("Unauthorized UID.");
    }
  } else {
    return;
  }
}


