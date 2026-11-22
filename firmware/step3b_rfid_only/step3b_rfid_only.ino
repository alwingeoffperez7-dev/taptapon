// TapTapon step 3b: ONLY the ESP32 and the RC522. Nothing else connected, no breadboard.
// Used to rule out wiring problems from the shared power rails.
//
// Results use the board's own small blue LED:
//   2 slow blinks at power-on  = reader answered (details in the Serial Monitor)
//   fast blinking forever      = reader NOT answering (wiring / soldering / 3.3V)
//   short flash every 2 s      = alive and scanning, waiting for a card
//   3 quick blinks             = CARD READ (number printed in the Serial Monitor, 115200 baud)
//
// Wiring (female-female, pin to pin):
//   RC522 3.3V->3V3, GND->GND, SDA->D5, SCK->D18, MOSI->D23, MISO->D19, RST->D15, IRQ->nothing
// Library: MFRC522 (by GithubCommunity)
#include <SPI.h>
#include <MFRC522.h>

const int PIN_ONBOARD_LED = 2;
const int PIN_RC522_SS = 5, PIN_RC522_RST = 15;   // SPI: SCK 18, MOSI 23, MISO 19

MFRC522 rfid(PIN_RC522_SS, PIN_RC522_RST);
bool readerOk = false;
unsigned long lastAliveAt = 0;

void blink(int times, int ms) {
  for (int i = 0; i < times; i++) {
    digitalWrite(PIN_ONBOARD_LED, HIGH); delay(ms);
    digitalWrite(PIN_ONBOARD_LED, LOW);  delay(ms);
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  pinMode(PIN_ONBOARD_LED, OUTPUT);
  Serial.println("\nStep 3b: ESP32 + RC522 only");

  SPI.begin();
  rfid.PCD_Init();
  delay(50);
  byte version = rfid.PCD_ReadRegister(MFRC522::VersionReg);
  // Genuine chips answer 0x91/0x92; common clones 0x88, 0x12 or 0xB2. Anything else is a wiring fault.
  readerOk = version == 0x91 || version == 0x92 || version == 0x88 || version == 0x12 || version == 0xB2;
  Serial.printf("RC522 version: 0x%02X -> %s\n", version,
                readerOk ? "reader answered correctly" : "WRONG ANSWER: wiring or soldering problem");

  bool selfTest = rfid.PCD_PerformSelfTest();     // clones may fail this but still read cards
  Serial.printf("Self-test: %s\n", selfTest ? "PASS" : "FAIL (ok on some clones)");
  rfid.PCD_Init();                                // self-test resets settings, so start again
  rfid.PCD_SetAntennaGain(MFRC522::RxGain_max);   // strongest antenna

  if (readerOk) {
    blink(2, 400);
    Serial.println("Scanning... hold the card FLAT on the reader for 2 seconds.");
  }
}

void loop() {
  if (!readerOk) { blink(1, 80); return; }        // fast blinking = reader problem

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String uid;
    for (byte i = 0; i < rfid.uid.size; i++) {
      if (rfid.uid.uidByte[i] < 0x10) uid += "0";
      uid += String(rfid.uid.uidByte[i], HEX);
    }
    uid.toUpperCase();
    rfid.PICC_HaltA();
    Serial.printf("CARD READ: %s\n", uid.c_str());
    blink(3, 100);
  }

  if (millis() - lastAliveAt > 2000) {            // short flash = still scanning
    lastAliveAt = millis();
    digitalWrite(PIN_ONBOARD_LED, HIGH); delay(20); digitalWrite(PIN_ONBOARD_LED, LOW);
  }
}
