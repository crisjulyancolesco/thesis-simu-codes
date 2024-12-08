#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Replace with your network credentials
const char* ssid = "OnePlus 10 Pro 5G";
const char* password = "A123456789";

// Supabase API information
String API_URL = "https://bqpbvkwmgllnlufoszdd.supabase.co/rest/v1/trash_bins";
String API_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImJxcGJ2a3dtZ2xsbmx1Zm9zemRkIiwicm9sZSI6ImFub24iLCJpYXQiOjE3MzI5NDkwMTksImV4cCI6MjA0ODUyNTAxOX0.4iWAnKoyyC_3LH1GEgk4Kn-Ezi5ilG2aS49zfEgvAxw";
String AUTH_BEARER = "Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImJxcGJ2a3dtZ2xsbmx1Zm9zemRkIiwicm9sZSI6InNlcnZpY2Vfcm9sZSIsImlhdCI6MTczMjk0OTAxOSwiZXhwIjoyMDQ4NTI1MDE5fQ.jQrnieDZlwDwPNnT2yM3U3LzLmLHjSGTHNmjoH608fE";

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

String fetchHttpResponse(const String& id, const String& column) {
  HTTPClient http;

  // Construct API URL with query parameters for filtering and selecting a specific column
  String queryUrl = API_URL + "?select=" + column + "&id=eq." + id;

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

bool postUpdateToServer(const String& id, int fullnessLevel) {
  HTTPClient http;

  String queryUrl = API_URL + "?id=eq." + id;

  http.begin(queryUrl);
  http.addHeader("apikey", API_KEY);
  http.addHeader("Authorization", AUTH_BEARER);

  // Construct JSON payload
  String jsonPayload = "{";
  jsonPayload += "\"fullness_level\": " + String(fullnessLevel);
  jsonPayload += "}";

  int httpCode = http.PATCH(jsonPayload); // Send PATCH request
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

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    // Specify the ID and column to fetch
    String id = "e5209cdb-7aa1-4cc0-990a-637eda6e1399"; // Replace with the actual ID
    String column = "fullness_level"; // Replace with the column to fetch

    String response = fetchHttpResponse(id, column);
    DynamicJsonDocument doc = processResponse(response);

    // Example: Extract and use data from the response
    if (!doc.isNull() && doc[0]) {
      int fullnessLevel = doc[0][column];
      Serial.print("Fetched fullness_level: ");
      Serial.println(fullnessLevel);

      // Example: Update server with new fullness_level for the same ID
      int newFullnessLevel = 80; // Replace with the desired fullness level
      postUpdateToServer(id, newFullnessLevel);
    }
  } else {
    Serial.println("Error in WiFi connection");
  }

  delay(5000); // Wait for 5 seconds before the next request
}
