// TapTapon step 3: RFID reader (+ the OLED and LED from steps 1-2). No Wi-Fi needed.
// Diagnostic version: shows the reader's version and self-test on the screen,
// and runs the antenna at maximum strength.
//
// Screen at startup:
//   Ver 0x91 / 0x92        = genuine chip      Ver 0x88 / 0x12 / 0xB2 = common clone, usually fine
//   Ver 0x00 / 0xFF / other = wiring problem (often MISO or a bad solder joint)
//   Self-test PASS         = chip works        FAIL = chip or 3.3V power problem (clones may FAIL yet still read cards)
// Then "Scanning..." with a moving dot: hold the card FLAT on the reader for 2 seconds.
// Card found -> big card number, LED blinks 3x.
//
// Wiring:
//   RC522: SDA->D5, SCK->D18, MOSI->D23, MISO->D19, RST->D15, 3.3V->3.3V rail, GND->GND rail, IRQ->nothing
//   LED:   D2 -> 220 ohm -> LED LONG leg; LED SHORT leg -> GND
//   OLED:  VCC->3.3V rail, GND->GND, SDA->D21, SCL->D22
// Libraries: MFRC522 (by GithubCommunity), Adafruit SSD1306
#include <SPI.h>
#include <Wire.h>
#include <MFRC522.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const int PIN_LED = 2;
const int PIN_RC522_SS = 5, PIN_RC522_RST = 15;   // SPI: SCK 18, MOSI 23, MISO 19

MFRC522 rfid(PIN_RC522_SS, PIN_RC522_RST);
Adafruit_SSD1306 oled(128, 64, &Wire, -1);
byte version = 0;
bool selfTestOk = false;
String lastCard = "";
int cardsSeen = 0;
unsigned long lastDrawAt = 0;
int dots = 0;

void blink(int times, int ms) {
  for (int i = 0; i < times; i++) {
    digitalWrite(PIN_LED, HIGH); delay(ms);
    digitalWrite(PIN_LED, LOW);  delay(ms);
  }
}

void draw() {
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.printf("STEP 3  Ver 0x%02X\n", version);
  oled.printf("Self-test: %s\n", selfTestOk ? "PASS" : "FAIL");
  if (lastCard.length()) {
    oled.setCursor(0, 24);
    oled.println("Card:");
    oled.setTextSize(2);
    oled.setCursor(0, 36);
    oled.println(lastCard);
    oled.setTextSize(1);
    oled.setCursor(0, 56);
    oled.printf("Taps: %d", cardsSeen);
  } else {
    oled.setCursor(0, 32);
    oled.print("Scanning");
    for (int i = 0; i < dots; i++) oled.print(".");
    oled.setCursor(0, 48);
    oled.println("Hold card flat 2 s");
  }
  oled.display();
}

void setup() {
  Serial.begin(115200);
  delay(300);
  pinMode(PIN_LED, OUTPUT);
  Wire.begin(21, 22);
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  SPI.begin();
  rfid.PCD_Init();
  delay(50);
  version = rfid.PCD_ReadRegister(MFRC522::VersionReg);

  // The self-test resets the chip's settings, so initialise again afterwards.
  selfTestOk = rfid.PCD_PerformSelfTest();
  rfid.PCD_Init();
  rfid.PCD_SetAntennaGain(MFRC522::RxGain_max);  // strongest antenna, helps weak boards and clones

  Serial.printf("\nStep 3: RC522 version 0x%02X, self-test %s\n", version, selfTestOk ? "PASS" : "FAIL");
  blink(2, 200);
  draw();
}

void loop() {
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String uid;
    for (byte i = 0; i < rfid.uid.size; i++) {
      if (rfid.uid.uidByte[i] < 0x10) uid += "0";
      uid += String(rfid.uid.uidByte[i], HEX);
    }
    uid.toUpperCase();
    rfid.PICC_HaltA();
    lastCard = uid;
    cardsSeen++;
    Serial.printf("Card tapped: %s\n", uid.c_str());
    draw();
    blink(3, 120);
  }

  if (millis() - lastDrawAt > 400) {   // moving dots prove the loop is running
    lastDrawAt = millis();
    dots = (dots + 1) % 4;
    if (!lastCard.length()) draw();
  }
}
