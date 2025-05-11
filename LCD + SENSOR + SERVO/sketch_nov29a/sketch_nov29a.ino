#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <ESP32Servo.h>
#include <Adafruit_PN532.h>

//For HTTP Requests
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// For Local Storage
#include <Preferences.h>

// For the time of Flight Sensor
#include <Adafruit_VL53L0X.h>

// For Sleep
#include "esp_sleep.h"

// For temperature Sensor   
#include "DHT.h"

Preferences preferences;

// Default Wi-Fi credentials
const char *defaultSSID = "nyx";
const char *defaultPassword = "B1234567";

const char* baseSSID = "Trash-Net";           // Base SSID for cascading
const char* apPassword = "password";          // Hotspot password
const int maxClients = 3;                     // Limit connections to 3

int hopLevel = 1;
bool isWifi = true;

// Supabase API information
String API_URL = "https://bqpbvkwmgllnlufoszdd.supabase.co/rest/v1";
String API_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImJxcGJ2a3dtZ2xsbmx1Zm9zemRkIiwicm9sZSI6ImFub24iLCJpYXQiOjE3MzI5NDkwMTksImV4cCI6MjA0ODUyNTAxOX0.4iWAnKoyyC_3LH1GEgk4Kn-Ezi5ilG2aS49zfEgvAxw";
String AUTH_BEARER = "Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImJxcGJ2a3dtZ2xsbmx1Zm9zemRkIiwicm9sZSI6InNlcnZpY2Vfcm9sZSIsImlhdCI6MTczMjk0OTAxOSwiZXhwIjoyMDQ4NTI1MDE5fQ.jQrnieDZlwDwPNnT2yM3U3LzLmLHjSGTHNmjoH608fE";

// Id of the trash can, must be specified each time new trashcan is added.
String Trash1_red_id = "5dc06acd-638d-44f8-88d5-bb57f9643a12"; // Replace with the actual ID
String Trash1_yellow_id = "93df8b18-5005-4e20-9c22-615d2f6b3d48";
String Trash1_green_id = "69d77faa-f302-4236-aca1-772afa94efee";

// Initialize the 20x4 I2C LCD
LiquidCrystal_I2C lcd1(0x27, 16, 2); // Address 0x25
LiquidCrystal_I2C lcd3(0x26, 16, 2); // Address 0x26
LiquidCrystal_I2C lcd2(0x25, 16, 2); // Address 0x27

// Create instances for each VL53L0X sensor
Adafruit_VL53L0X lox1 = Adafruit_VL53L0X();
Adafruit_VL53L0X lox2 = Adafruit_VL53L0X();
Adafruit_VL53L0X lox3 = Adafruit_VL53L0X();

// X Shut Pins
#define XSHUT_PIN_1 13
#define XSHUT_PIN_2 16
#define XSHUT_PIN_3 12

// Address of ToF
#define LOX1_ADDRESS 0x31
#define LOX2_ADDRESS 0x32
#define LOX3_ADDRESS 0x30

// Define pin for Servo Motor
#define SERVO1_PIN 27
#define SERVO2_PIN 32
#define SERVO3_PIN 33

// Define I2C pins for ESP32
#define SDA_PIN 21
#define SCL_PIN 22

// Pin for relay
#define RELAY_PIN 15 

// Create a Servo object
Servo servo1;
Servo servo2;
Servo servo3;

const int analogPin = 34;
const int THRESHOLD = 2630;
const int switchPin = 25;

// Temperature and Fan Pins and Setup
#define DHTPIN 18        // GPIO connected to DHT11 data pin
#define DHTTYPE DHT11    // DHT 11
#define FAN_PIN 19
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8

DHT dht(DHTPIN, DHTTYPE);

// Create the PN532 object
Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);

// Define the authorized UID (pass)
const uint8_t authorizedUIDLength = 4;

// Global variables
int upperLimit;
int lowerLimit;

bool justUnlock = false;

// Structure to store time
struct Time {
    int hour;
    int minute;
    int second;
    int microsecond;
};

// Function to measure distance from a VL53L0X sensor
float getDistance(Adafruit_VL53L0X &lox) {
  VL53L0X_RangingMeasurementData_t measure;
  lox.rangingTest(&measure, false);

  if (measure.RangeStatus != 4) { // If valid reading
    return measure.RangeMilliMeter / 10.0; // Convert to cm
  } else {
    return -1; // Out of range
  }
}

bool initializeSensor(Adafruit_VL53L0X &sensor, uint8_t address, const char* sensorName) {
  int attempts = 0;
  while (attempts < 5) {
    if (sensor.begin(address)) {
      Serial.print(sensorName);
      Serial.println(" initialized.");
      return true;
    }
    Serial.print("Failed to initialize ");
    Serial.print(sensorName);
    Serial.println(". Retrying...");
    attempts++;
    delay(1000); // Wait before retrying
  }
  return false; // Failed after 5 attempts
}

// Sets The Id for the ToF Sensors
void setID() {
  // Reset all sensors
  digitalWrite(XSHUT_PIN_1, LOW);
  digitalWrite(XSHUT_PIN_2, LOW);
  digitalWrite(XSHUT_PIN_3, LOW);
  delay(10);

  // Initialize sensor 1
  digitalWrite(XSHUT_PIN_1, HIGH);
  delay(10);
  if (!initializeSensor(lox1, LOX1_ADDRESS, "Sensor 1")) {
    Serial.println("Sensor 1 failed after 5 attempts. Restarting...");
    ESP.restart(); // Restart the ESP32 if sensor fails to initialize
  }

  // Initialize sensor 2
  digitalWrite(XSHUT_PIN_2, HIGH);
  delay(10);
  if (!initializeSensor(lox2, LOX2_ADDRESS, "Sensor 2")) {
    Serial.println("Sensor 2 failed after 5 attempts. Restarting...");
    ESP.restart(); // Restart the ESP32 if sensor fails to initialize
  }

  // Initialize sensor 3
  digitalWrite(XSHUT_PIN_3, HIGH);
  delay(10);
  if (!initializeSensor(lox3, LOX3_ADDRESS, "Sensor 3")) {
    Serial.println("Sensor 3 failed after 5 attempts. Restarting...");
    ESP.restart(); // Restart the ESP32 if sensor fails to initialize
  }
}

// If cannot connect to WiFi, Scan for ESP32 connection
void scanAndConnectToESP32() {
  Serial.println("Scanning for ESP32 hotspots...");

  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_STA);
  delay(1000);
  
  int numNetworks = WiFi.scanNetworks();
  Serial.print("Networks found: ");
  Serial.println(numNetworks);

  int bestHop = 100;
  String bestSSID = "";

  for (int i = 0; i < numNetworks; i++) {
    String foundSSID = WiFi.SSID(i);
    
    String hopString = foundSSID.substring(strlen(baseSSID) + 1);
    if (hopString.length() > 0 && isDigit(hopString[0])) {
        int foundHop = hopString.toInt();
        Serial.println(foundHop);
        if (foundHop > 0 && foundHop < bestHop) {
            bestHop = foundHop;
            bestSSID = foundSSID;
        }
    }
  }

  if (bestSSID.length() > 0) {
    Serial.println("Connecting to: " + bestSSID);
    WiFi.begin(bestSSID.c_str(), apPassword);
    delay(15000);
    hopLevel = bestHop + 1;
    if (WiFi.status() == WL_CONNECTED){
      Serial.println("Connected");
      isWifi = false;
      return;
    } else {
      Serial.println("Connection Failed");
      return;
    }
  }
  Serial.println("No valid ESP32 networks found.");
}

// Check WiFi Connection Status
void connectionCheck() {
    if (WiFi.status() == WL_CONNECTED) {
      return;
    } else if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Connection Lost..");
      Serial.println("Trash restarting");
      delay(5000);
      ESP.restart(); 
    }
  delay(20000);
}

// Starts Hotspot
void startAP() {
  if (WiFi.status() == WL_CONNECTED){ 
    Serial.println("Starting AP");
    IPAddress staIP = WiFi.localIP();
    
    int lastByte = isWifi ? 1 : staIP[3];

    // Unique SSID
    String newSSID = String(baseSSID) + "-" + String(hopLevel) + "-" + String(lastByte);

    // Generate dynamic AP IP
    IPAddress ap_ip(192, 168, hopLevel + 4, 4 * lastByte - 3);
    IPAddress ap_mask(255, 255, 255, 0);
    IPAddress ap_leaseStart(192, 168, hopLevel + 4, 4 * lastByte - 2);
    IPAddress ap_dns(8, 8, 4, 4);

    WiFi.AP.begin();
    WiFi.AP.config(ap_ip, ap_ip, ap_mask, ap_leaseStart, ap_dns);
    WiFi.AP.create(newSSID.c_str(), apPassword);

    Serial.println("AP Started: " + newSSID);
    Serial.print("AP IP: "); Serial.println(ap_ip);
    Serial.print("DHCP Lease Start: "); Serial.println(ap_leaseStart);
    WiFi.AP.enableNAPT(true);
  } 
  else {
    Serial.println("Cannot Start AP Without WiFi");
  }
}

//Connect to WiFi
void initializeWiFi() {
  preferences.begin("WiFiCreds", true);

  // Fetch to local Storage
  String savedSSID = preferences.getString("ssid", "");
  String savedPassword = preferences.getString("password", "");

  preferences.end();

  Serial.print("Saved SSID and Password: ");
  Serial.print(savedSSID);
  Serial.println(savedPassword);

  if (!savedSSID.isEmpty() || !savedPassword.isEmpty()) {
    Serial.println("Using saved WiFi credentials...");
    WiFi.begin(savedSSID, savedPassword);
    delay(15000);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nConnection Failed! Trying default credentials...");
    WiFi.disconnect(true, true);
    delay(1000);
    
    WiFi.begin(defaultSSID, defaultPassword);
    
    delay(15000);
    
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("\nFailed to connect to default WiFi. Scanning for ESP32...");
      scanAndConnectToESP32();
    }
  }
  
  startAP(); 
}

bool previousSwitchState = LOW;

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  delay(1000);

  // Turn on relay first
  pinMode(RELAY_PIN, OUTPUT);
  delay(1000);
  digitalWrite(RELAY_PIN, HIGH);
  Serial.println(F("Relay Turned On"));

  pinMode(XSHUT_PIN_1, OUTPUT);
  pinMode(XSHUT_PIN_2, OUTPUT);
  pinMode(XSHUT_PIN_3, OUTPUT);

  Serial.println(F("VL53L0X Shutdown pins inited..."));

  digitalWrite(XSHUT_PIN_1, LOW);
  digitalWrite(XSHUT_PIN_2, LOW);
  digitalWrite(XSHUT_PIN_3, LOW);

  delay(100);

  digitalWrite(XSHUT_PIN_1, HIGH);
  digitalWrite(XSHUT_PIN_2, HIGH);
  digitalWrite(XSHUT_PIN_3, HIGH);

  delay(100);

  digitalWrite(XSHUT_PIN_1, LOW);
  digitalWrite(XSHUT_PIN_2, LOW);
  digitalWrite(XSHUT_PIN_3, LOW);

  Serial.println(F("VL53L0X Both in reset mode...(pins are low)"));
  
  Serial.println(F("VL53L0X Starting..."));
  setID();

  // Attach the Servo to the specified pin and set initial position
  servo1.attach(SERVO1_PIN);
  servo2.attach(SERVO2_PIN);
  servo3.attach(SERVO3_PIN);

  if (!servo3.attach(SERVO3_PIN)) {
    Serial.println("Failed to attach servo3!");
  }
  
  // Close All Bins
  servo1.write(0);
  servo2.write(0);
  servo3.write(0);

  delay(5000);

  // Opens All Bins
  servo1.write(180);
  servo2.write(180);
  servo3.write(180);

  pinMode(switchPin, INPUT);
  bool currentSwitchState = digitalRead(switchPin);
  previousSwitchState = currentSwitchState;
  
  initializeWiFi();

  //Update Sleeping in tables
  updateToServer(Trash1_red_id, "is_sleep", false, "bins");
  updateToServer(Trash1_yellow_id, "is_sleep", false, "bins");
  updateToServer(Trash1_green_id, "is_sleep", false, "bins");

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  delay(1000);

  dht.begin();
  Serial.println("Temperature Sensor Initialized");
  ledcAttach(FAN_PIN, PWM_FREQ, PWM_RESOLUTION);

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

  //Initialize each VL53L0X sensor
  if (!lox1.begin(0x30)) { // Set custom I2C address for lox1
    Serial.println("Failed to initialize VL53L0X sensor 1!");
    while (1);
  }

  if (!lox2.begin(0x31)) { // Set custom I2C address for lox2
    Serial.println("Failed to initialize VL53L0X sensor 2!");
    while (1);
  }

  if (!lox3.begin(0x32)) { // Set custom I2C address for lox3
    Serial.println("Failed to initialize VL53L0X sensor 3!");
    while (1);
  }
}

// Function to Fetch a specific table from trash_bins table from DB
String fetchHttpResponse(const String& filterKey, const String& filterValue, const String& column, const String& table) {
  if (WiFi.status() != WL_CONNECTED) {
    connectionCheck();
  }

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
  connectionCheck();

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

// Create data to the server
bool createToServer(const String& binId, int fillLevel, bool isFull, const String& table) {
  connectionCheck();

  HTTPClient http;

  // Construct the query URL
  String queryUrl = API_URL + "/" + table;

  // Initialize the HTTP request
  http.begin(queryUrl);
  http.addHeader("apikey", API_KEY);
  http.addHeader("Authorization", AUTH_BEARER);
  http.addHeader("Content-Type", "application/json");

  // Construct JSON payload with bin_id, fill_level, and isFull
  String jsonPayload = "{";
  jsonPayload += "\"bin_id\": \"" + binId + "\",";
  jsonPayload += "\"fill_level\": " + String(fillLevel) + ",";
  jsonPayload += "\"is_full\": " + String(isFull ? "true" : "false");
  jsonPayload += "}";

  // Send the INSERT request to create a new record
  int httpCode = http.POST(jsonPayload);  // Send POST request
  if (httpCode == 201) {  // 201 Created
    Serial.println("Record creation successful");
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

bool detectingFullness(const String& id, bool &isLocked) {
  if (WiFi.status() == WL_CONNECTED) {
    // Specify the ID and columns to fetch
    String columns = "is_locked,upper_limit,lower_limit,ssid,password,time_on,time_off,current_bin_time";

    // Fetch the HTTP response for the specified ID and columns
    String response = fetchHttpResponse("id", id, columns, "bins");  // Assuming this fetches the HTTP response correctly
    DynamicJsonDocument doc = processResponse(response);  // Assuming this processes the response correctly

    // Extract and use data from the response
    if (!doc.isNull() && doc.size() > 0) {
      if (doc[0].containsKey("is_locked") && doc[0].containsKey("upper_limit") && doc[0].containsKey("lower_limit")) {
        // Update global variables
        bool lockStatus = doc[0]["is_locked"]; 
        upperLimit = doc[0]["upper_limit"];
        lowerLimit = doc[0]["lower_limit"];

        if (lockStatus != isLocked) {
          Serial.println("Just Unlocked");
          justUnlock = true;
        }
        
        // Get the sleep schedule
        String timeOn = doc[0]["time_on"];
        String timeOff = doc[0]["time_off"];
        String currentTime = doc[0]["current_bin_time"];
        
        Serial.println("Time: " + timeOn + ", " + timeOff + ", " + currentTime);

        checkSleep(currentTime, timeOn, timeOff);

        String fetchedSsid = doc[0]["ssid"];
        String fetchedPassword = doc[0]["password"];

        if (fetchedSsid && fetchedPassword) {
          const char *ssid = doc[0]["ssid"];
          const char *password = doc[0]["password"];

          preferences.begin("WiFiCreds", false);

          // Check if SSID or password is different from the stored value
          String storedSSID = preferences.getString("ssid", "");
          String storedPassword = preferences.getString("password", "");

          if (String(ssid) != storedSSID) { {
            preferences.putString("ssid", ssid);
            Serial.println("New SSID Saved");
          }

          if (String(password) != storedPassword) {
            preferences.putString("password", password);
            Serial.println("New Password Saved");
          }

          preferences.end();
        }

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
    connectionCheck();
  }
  return true;  // Default return value in case of error
  }
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
    connectionCheck();
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

unsigned long previousCheckTime = 0; // Last time trash bins were checked
unsigned long previousCheckTime2 = 0;
const unsigned long checkInterval = 8 * 60 * 1000;
const unsigned long checkInterval2 = 1 * 60 * 1000;

void loop() {
  unsigned long currentTime = millis();

  if (currentTime - previousCheckTime >= checkInterval || previousCheckTime == 0 || justUnlock) {
    previousCheckTime = currentTime; // Update the last check time

    Serial.println("Checking trash bins...");
    checkTrashBin(Trash1_red_id, lox1, lcd1, servo1, fullCount1, isLocked1);
    checkTrashBin(Trash1_yellow_id, lox2, lcd2, servo2, fullCount2, isLocked2);
    checkTrashBin(Trash1_green_id, lox3, lcd3, servo3, fullCount3, isLocked3);

    justUnlock = false;
  }
  
  if (currentTime - previousCheckTime2 >= checkInterval2) {
    previousCheckTime2 = currentTime;
    detectingFullness(Trash1_red_id, isLocked1);
    detectingFullness(Trash1_yellow_id, isLocked2);
    detectingFullness(Trash1_green_id, isLocked3);
    controlFan();
  }

  checkSwitch();
  handleNFC();
}

void checkTrashBin(String trashId, Adafruit_VL53L0X &tofSensor, LiquidCrystal_I2C lcd, Servo &servo, int &fullCount, bool &isLocked) {
  if (detectingFullness(trashId, isLocked)) {
    // Unlock the trash bin by rotating the servo
    controlServo(servo, isLocked, true, trashId);
    delay(1000);

    // Get the distance from the ToF sensor
    float distance = getDistance(tofSensor);
    float percentage;

    if (distance == -1) {
      // If distance reading is invalid
      lcd.setCursor(0, 0);
      lcd.print("Sensor Error!     ");
    }

    if (distance > upperLimit) {
      percentage = 0;
      fullCount = 0;
      lcd.setCursor(0, 2);
      lcd.print("                   ");
    } else if (distance < lowerLimit) {
      percentage = 100;
      fullCount++;
      if (fullCount >= 3) {
        lcd.setCursor(0, 2);
        lcd.print("FULL               ");
        createToServer(trashId, 100, true, "sensor_history");
      }
    } else {
      percentage = (1 - ((distance - lowerLimit) / (upperLimit - lowerLimit))) * 100;
      fullCount = 0;
      lcd.setCursor(0, 2);
      lcd.print("                   ");
    }

    // Display the fullness level on the LCD
    lcd.setCursor(0, 0);
    lcd.print("Fullness Level: ");
    lcd.setCursor(4, 1);
    lcd.print(percentage);
    lcd.print(" %     ");

    // Print to Serial Monitor for debugging
    Serial.print("Percentage: ");
    Serial.print(percentage);
    Serial.println(" %");
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");

    // Send the data to the server
    createToServer(trashId, percentage, false, "sensor_history");

  } else {
    lcd.setCursor(0, 2);
    lcd.print("LOCKED: NFC REQ.");
    controlServo(servo, isLocked, false, trashId);
    delay(1000);
    handleNFC();
  }

  delay(500); // Adjust for delay in each bin
}

void handleNFC() {
  uint8_t success;
  uint8_t uid[7];
  uint8_t uidLength;

  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 5000);
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

      // Open
      servo1.write(180);
      servo2.write(180);
      servo3.write(180);
      
      updateToServer(Trash1_red_id, "is_locked", false, "bins");
      updateToServer(Trash1_yellow_id, "is_locked", false, "bins");
      updateToServer(Trash1_green_id, "is_locked", false, "bins");

      fullCount1 = 0;
      fullCount2 = 0;
      fullCount3 = 0;

      isLocked1 = false;
      isLocked2 = false;
      isLocked3 = false;

      justUnlock = true;

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

// Function to parse time string "HH:MM:SS.MMMMMM"
Time parseTime(String timeStr) {
    Time t;
    sscanf(timeStr.c_str(), "%d:%d:%d.%d", &t.hour, &t.minute, &t.second, &t.microsecond);
    return t;
}

// Function to convert Time struct to total microseconds
uint64_t timeToMicroseconds(Time t) {
    return ((uint64_t)t.hour * 3600 + t.minute * 60 + t.second) * 1000000 + t.microsecond;
}

// Checks time and sleep
void checkSleep(String currentTime, String timeOn, String timeOff) {
    uint64_t sleepDuration = calculateSleepDuration(currentTime, timeOn, timeOff);
    
    if (sleepDuration > 0) {
        Serial.println("Sleeping time");
        // Opens All Bins
        controlServo(servo1, isLocked1, true, Trash1_red_id);
        controlServo(servo2, isLocked2, true, Trash1_yellow_id);
        controlServo(servo3, isLocked3, true, Trash1_green_id);
        delay(2000);

        //Update Sleeping in tables
        updateToServer(Trash1_red_id, "is_sleep", true, "bins");
        updateToServer(Trash1_yellow_id, "is_sleep", true, "bins");
        updateToServer(Trash1_green_id, "is_sleep", true, "bins");
        
        digitalWrite(RELAY_PIN, LOW);
        gpio_hold_en((gpio_num_t)RELAY_PIN);
        delay(5000);

        Serial.printf("Sleeping for %.2f seconds...\n", sleepDuration / 1000000.0);
        esp_sleep_enable_timer_wakeup(sleepDuration);
        esp_deep_sleep_start();
    } else {
        Serial.println("Device is within operating time");
    }
}

// Function to calculate sleep duration
uint64_t calculateSleepDuration(String currentTimeStr, String timeOnStr, String timeOffStr) {
    Time currentTime = parseTime(currentTimeStr);
    Time timeOn = parseTime(timeOnStr);
    Time timeOff = parseTime(timeOffStr);
    
    uint64_t currentMicro = timeToMicroseconds(currentTime);
    uint64_t onMicro = timeToMicroseconds(timeOn);
    uint64_t offMicro = timeToMicroseconds(timeOff);
    
    if (offMicro < onMicro) {
        offMicro += 86400000000ULL;
    }

    if (currentMicro < onMicro) {
        return onMicro - currentMicro;
    }

    else if (currentMicro >= offMicro) {
        return (86400000000ULL - currentMicro) + onMicro;
    }
    else {
        return 0;
    }
}

void controlServo(Servo &servo, bool &isLocked, bool commandToOpen, const String& id) {
  int adcValue = analogRead(analogPin);
  Serial.print("ADC Value: ");
  Serial.println(adcValue);

  if (adcValue <= THRESHOLD) {
    // Force open ALL servos if ADC is low
    if (isLocked1) {
      servo1.write(180);
      isLocked1 = false;
      updateToServer(Trash1_red_id, "is_locked", false, "bins");
      Serial.println("ADC low - Forcing servo1 open.");
    }
    if (isLocked2) {
      servo2.write(180);
      isLocked2 = false;
      updateToServer(Trash1_yellow_id, "is_locked", false, "bins");
      Serial.println("ADC low - Forcing servo2 open.");
    }
    if (isLocked3) {
      servo3.write(180);
      isLocked3 = false;
      updateToServer(Trash1_green_id, "is_locked", false, "bins");
      Serial.println("ADC low - Forcing servo3 open.");
    }
  } else {
    // ADC is above threshold – allow manual control
    if (commandToOpen && isLocked) {
      servo.write(180);  // Open
      isLocked = false;
      updateToServer(id, "is_locked", false, "bins");
      Serial.println("Opened servo.");
    } else if (!commandToOpen && !isLocked) {
      servo.write(0);    // Close
      isLocked = true;
      updateToServer(id, "is_locked", true, "bins");
      Serial.println("Closed servo.");
    }
  }
}

void checkSwitch() {
  bool currentSwitchState = digitalRead(switchPin);
  if (currentSwitchState != previousSwitchState) {
    // Switch has changed state - open all servos
    Serial.println("Manual switch toggled - opening all servos.");
    servo1.write(180);
    servo2.write(180);
    servo3.write(180);

    isLocked1 = false;
    isLocked2 = false;
    isLocked3 = false;

    fullCount1 = 0;
    fullCount2 = 0;
    fullCount3 = 0;

    justUnlock = true;

    previousSwitchState = currentSwitchState;
    
    updateToServer(Trash1_red_id, "is_locked", false, "bins");
    updateToServer(Trash1_yellow_id, "is_locked", false, "bins");
    updateToServer(Trash1_green_id, "is_locked", false, "bins");
    return;
  }
}

void controlFan() {
  float tempC = dht.readTemperature(); // Read temperature in Celsius

  if (isnan(tempC)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Temperature: ");
  Serial.print(tempC);
  Serial.println("°C");

  int duty;

  if (tempC < 40) {
    duty = 0; // Fan off if temperature is below 40°C
  } else {
    // Map temperature from 40°C upwards to the duty cycle range (130 to 255)
    duty = map(constrain(tempC, 40, 55), 40, 55, 130, 255);
  }
  Serial.print(duty);
  ledcWrite(FAN_PIN, duty);

  delay(1000); // Wait a second before reading again
}


