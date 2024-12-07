#include <Wire.h>
#include <Adafruit_PN532.h>

// Define I2C pins for ESP32
#define SDA_PIN 26
#define SCL_PIN 17

// Define LED pin
#define LED_PIN 2

// Create the PN532 object
Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);

// Define the authorized UID (pass)
uint8_t authorizedUID[] = {0xFA, 0xD6, 0xAF, 0x80};
const uint8_t authorizedUIDLength = 4;

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);

  // Initialize the LED pin as OUTPUT
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // Ensure LED is off at start

  // Initialize PN532
  nfc.begin();
  
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("PN532 not found!");
    while (1);
  }
  
  nfc.SAMConfig(); // Configure PN532 to read NFC tags
  Serial.println("Waiting for an NFC tag...");
}

void loop() {
  uint8_t success;
  uint8_t uid[7];  // Buffer to store the UID
  uint8_t uidLength;

  // Try to read an NFC tag
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
      Serial.println("Authorized UID! Turning LED ON.");
      digitalWrite(LED_PIN, HIGH); // Turn LED ON
    } else {
      Serial.println("Unauthorized UID. Turning LED OFF.");
      digitalWrite(LED_PIN, LOW); // Turn LED OFF
    }

    delay(1000);  // Delay before reading again
  }
}

// Function to compare two UIDs
bool compareUIDs(uint8_t *uid1, uint8_t *uid2, uint8_t length) {
  for (uint8_t i = 0; i < length; i++) {
    if (uid1[i] != uid2[i]) {
      return false; // Mismatch found
    }
  }
  return true; // UIDs match
}
