// TapTapon test 1: RFID reader + LED + OLED screen. No Wi-Fi needed.
// Same pins as the real bin (CLAUDE.md §2).
//
// What you should see:
//   Power on       -> LED blinks 2x; screen shows "RC522 OK, tap a card" (or "NOT FOUND")
//   RC522 missing  -> LED keeps blinking fast (check wiring / soldering / 3.3V)
//   Tap a card     -> UID shown on screen + Serial Monitor (115200), LED blinks 3x
// The OLED is optional: the test still runs without it (Serial Monitor shows everything).
//
// Wiring:
//   RC522: SDA->D5, SCK->D18, MOSI->D23, MISO->D19, RST->D15, 3.3V->3V3, GND->GND, IRQ->nothing
//   LED:   D2 -> 220 ohm -> LED LONG leg; LED SHORT leg -> GND
//   OLED:  VCC->3V3, GND->GND, SDA->D21, SCL->D22
// Libraries: MFRC522 (by GithubCommunity), Adafruit SSD1306 (say yes to Adafruit GFX)
#include <SPI.h>
#include <Wire.h>
#include <MFRC522.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const int PIN_LED = 2;                            // also the board's onboard blue LED
const int PIN_RC522_SS = 5, PIN_RC522_RST = 15;   // SPI: SCK 18, MOSI 23, MISO 19
const int PIN_OLED_SDA = 21, PIN_OLED_SCL = 22;

MFRC522 rfid(PIN_RC522_SS, PIN_RC522_RST);
Adafruit_SSD1306 oled(128, 64, &Wire, -1);
bool readerOk = false, screenOk = false;
int cardsSeen = 0;

void blink(int times, int ms) {
  for (int i = 0; i < times; i++) {
    digitalWrite(PIN_LED, HIGH); delay(ms);
    digitalWrite(PIN_LED, LOW);  delay(ms);
  }
}

// Shows a title line and up to two lines of detail; a no-op if the screen isn't connected.
void show(const String& line1, const String& line2 = "", const String& line3 = "") {
  if (!screenOk) return;
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.println("TapTapon test 1");
  oled.setCursor(0, 20);
  oled.println(line1);
  oled.setTextSize(2);          // the card number big enough to read at arm's length
  oled.setCursor(0, 34);
  oled.println(line2);
  oled.setTextSize(1);
  oled.setCursor(0, 56);
  oled.println(line3);
  oled.display();
}

void setup() {
  Serial.begin(115200);
  delay(300);
  pinMode(PIN_LED, OUTPUT);
  Serial.println("\nTest 1: RFID + LED + OLED");
  blink(2, 200);  // proves the LED works before anything else

  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
  screenOk = oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  Serial.println(screenOk ? "OLED: OK" : "OLED: not found at 0x3C (check VCC 3V3, GND, SDA D21, SCL D22)");

  SPI.begin();
  rfid.PCD_Init();
  // The version register reads 0x00 or 0xFF when the reader isn't connected properly.
  byte version = rfid.PCD_ReadRegister(MFRC522::VersionReg);
  readerOk = version != 0x00 && version != 0xFF;
  Serial.printf("RC522 version: 0x%02X -> %s\n", version,
                readerOk ? "OK, tap a card" : "NOT FOUND: check wires, solder joints, 3.3V");
  if (readerOk) show("RC522 OK. Tap a card", "", "");
  else show("RC522 NOT FOUND", "check", "wires, solder, 3.3V");
}

void loop() {
  if (!readerOk) {        // fast blinking = reader problem, so you can tell without the screen
    blink(1, 80);
    return;
  }
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;

  String uid;
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  rfid.PICC_HaltA();
  cardsSeen++;

  Serial.printf("Card tapped: %s\n", uid.c_str());  // the UID you register on the admin page
  show("Card tapped:", uid, "Taps so far: " + String(cardsSeen));
  blink(3, 120);
}
