#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 21   // SDA (SS) pin for ESP32
#define RST_PIN 22  // Reset pin for ESP32
 
MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class

// Array of valid NUIDs (RFID tags) in hexadecimal format
byte validTags[][4] = {
  {0x49, 0x2B, 0x33, 0x0F}, // Valid tag example
  // You can add more valid tags like this:
  {0xC3, 0x5F, 0xD7, 0x12},
  {0xFA, 0xD6, 0xAF, 0x80},
  {0xFA, 0xD6, 0xAF, 0x80},
};

void setup() { 
  Serial.begin(9600);
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522 

  Serial.println(F("This code scans the MIFARE Classic NUID."));
}
 
void loop() {

  // Reset the loop if no new card present on the sensor/reader.
  if ( ! rfid.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been read
  if ( ! rfid.PICC_ReadCardSerial())
    return;

  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  // Check if the PICC is of the MIFARE Classic type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&  
    piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
    piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  // Check if the read tag is valid
  bool isValid = false;
  for (byte i = 0; i < sizeof(validTags) / sizeof(validTags[0]); i++) {
    bool match = true;
    for (byte j = 0; j < 4; j++) {
      if (rfid.uid.uidByte[j] != validTags[i][j]) {
        match = false;
        break;
      }
    }
    if (match) {
      isValid = true;
      break;
    }
  }

  if (isValid) {
    Serial.println(F("This is a valid card!"));
    Serial.print(F("The NUID tag is: "));
    Serial.print(F("In hex: "));
    printHex(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();
    Serial.print(F("In dec: "));
    printDec(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();
  } else {
    Serial.println(F("This card is not valid."));
  }

  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
}

/**
 * Helper routine to dump a byte array as hex values to Serial. 
 */
void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

/**
 * Helper routine to dump a byte array as dec values to Serial.
 */
void printDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(' ');
    Serial.print(buffer[i], DEC);
  }
}
