#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Replace with your network credentials
const char* ssid = "OnePlus 10 Pro 5G";
const char* password = "A123456789";

// Supabase API information
String API_URL = "https://uvhbiqrgjfklphcwcxqa.supabase.co/rest/v1/led_table?select=*";
String API_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InV2aGJpcXJnamZrbHBoY3djeHFhIiwicm9sZSI6InNlcnZpY2Vfcm9sZSIsImlhdCI6MTczMjk0NTg1MCwiZXhwIjoyMDQ4NTIxODUwfQ.3GqAwBe3BThg-UI1-Hkm-ltdWuGXAmebVsN14SejlJY";
String AUTH_BEARER = "Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InV2aGJpcXJnamZrbHBoY3djeHFhIiwicm9sZSI6ImFub24iLCJpYXQiOjE3MzI5NDU4NTAsImV4cCI6MjA0ODUyMTg1MH0.Upidie78d18cn1iX5LdIvaiKM7kKB18nxLsgUKkMlrg";

// Define LED pins
#define LED1 2 // GPIO2 (Default onboard LED for ESP32)

void setup() {
  // Initialize LED pins
  pinMode(LED1, OUTPUT);
  digitalWrite(LED1, LOW); // Initially OFF

  // Connect to WiFi
  Serial.begin(115200);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Begin HTTPS request
    http.begin(API_URL);
    http.addHeader("apikey", API_KEY);
    http.addHeader("Authorization", AUTH_BEARER);

    int httpCode = http.GET(); // Send GET request
    if (httpCode == 200) {
      String payload = http.getString();
      Serial.println(payload); // Print fetched data

      // Parse JSON payload
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, payload);
      if (!error) {
        // Assuming the first record controls LED_1
        const char* led = doc[0]["led"];
        bool value = doc[0]["value"];

        // Control LED based on fetched data
        if (String(led) == "LED_1") {
          digitalWrite(LED1, value ? HIGH : LOW);
        }
      } else {
        Serial.println("Failed to parse JSON");
      }
    } else {
      Serial.print("Error in HTTP request: ");
      Serial.println(httpCode);
    }

    http.end(); // Close connection
  } else {
    Serial.println("Error in WiFi connection");
  }

  delay(5000); // Wait for 5 seconds before the next request
}
