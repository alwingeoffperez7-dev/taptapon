// TapTapon parts tester: checks each part on its own, no Wi-Fi or Firebase needed.
// Same pins as the real bin (CLAUDE.md §2), so the wiring you test here is the wiring you keep.
//
// The OLED is a live readout: distance, bottle sensor, button and the last RFID card.
// Touch the parts and watch the screen:
//   - hand in front of the HC-SR04   -> distance changes
//   - break the beam / press GPIO 32 -> short beep, "Bottle: DETECTED"
//   - press the residual button      -> servo opens 90 degrees for 1 s, then closes
//   - tap an RFID card               -> UID shown, beep + 3 LED blinks
// Serial Monitor (115200) shows the same, plus commands: b = buzzer, l = LED, s = servo.
//
// Libraries (Arduino IDE > Library Manager): ESP32Servo, MFRC522, Adafruit SSD1306 (installs Adafruit GFX too).
#include <SPI.h>
#include <Wire.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const int PIN_TRIG = 26, PIN_ECHO = 27;
const int PIN_SERVO = 13;
const int PIN_IR = 32;            // bottle sensor: break-beam, obstacle module, or a test button (LOW = detected)
const int PIN_LED = 2, PIN_BUZZER = 33;
const int PIN_RESIDUAL_BTN = 14;
const int PIN_RC522_SS = 5, PIN_RC522_RST = 15;  // SPI: SCK 18, MOSI 23, MISO 19
const int PIN_OLED_SDA = 21, PIN_OLED_SCL = 22;  // ESP32 default I2C pins

Adafruit_SSD1306 oled(128, 64, &Wire, -1);
MFRC522 rfid(PIN_RC522_SS, PIN_RC522_RST);
Servo flap;

bool haveOled = false, haveRfid = false;
byte rfidVersion = 0;
float distanceCm = -1;
String lastCard = "-", lastEvent = "ready";
int lastIr = HIGH, lastBtn = HIGH;
unsigned long lastMeasureAt = 0, lastDrawAt = 0;

void beep(int ms, int hz) {
  long halfUs = 500000L / hz;
  for (long t = 0; t < ms * 1000L; t += halfUs * 2) {
    digitalWrite(PIN_BUZZER, HIGH); delayMicroseconds(halfUs);
    digitalWrite(PIN_BUZZER, LOW);  delayMicroseconds(halfUs);
  }
}

void blink(int times) {
  for (int i = 0; i < times; i++) { digitalWrite(PIN_LED, HIGH); delay(120); digitalWrite(PIN_LED, LOW); delay(120); }
}

void event(const String& text) {
  lastEvent = text;
  Serial.println(text);
}

float readDistanceCm() {
  digitalWrite(PIN_TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  unsigned long us = pulseIn(PIN_ECHO, HIGH, 30000);
  return us ? us / 58.0 : -1;  // -1 = no echo
}

void openFlap() {
  event("Servo: open 90");
  flap.write(90);
  delay(1000);
  flap.write(0);
  event("Servo: closed (if the board reset, USB power is too weak)");
}

void checkCard() {
  if (!haveRfid || !rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;
  String uid;
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  rfid.PICC_HaltA();
  lastCard = uid;
  event("Card: " + uid);
  beep(80, 1800);
  blink(3);
}

void draw() {
  if (!haveOled) return;
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);
  oled.println("TapTapon part test");
  oled.println();
  if (distanceCm < 0) oled.println("Dist:  no echo");
  else oled.printf("Dist:  %.1f cm\n", distanceCm);
  oled.printf("Bottle: %s\n", lastIr == LOW ? "DETECTED" : "clear");
  oled.printf("Button: %s\n", lastBtn == LOW ? "PRESSED" : "up");
  if (haveRfid) oled.printf("Card:  %s\n", lastCard.c_str());
  else oled.println("Card:  RC522 not found");
  oled.println(lastEvent.substring(0, 21));  // one line = 21 characters
  oled.display();
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\nTapTapon parts tester");
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_IR, INPUT_PULLUP);
  pinMode(PIN_RESIDUAL_BTN, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  // OLED: list everything on the I2C bus first, so a wrong address or loose wire is obvious.
  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
  Serial.print("I2C devices found:");
  int found = 0;
  for (byte a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) { Serial.printf(" 0x%02X", a); found++; }
  }
  Serial.println(found ? "" : " none (check OLED wiring: SDA 21, SCL 22, VCC 3.3V)");
  haveOled = oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  Serial.println(haveOled ? "OLED: OK" : "OLED: not found at 0x3C");

  // RFID: the version register reads 0x00 or 0xFF when the reader isn't wired right.
  SPI.begin();
  rfid.PCD_Init();
  rfidVersion = rfid.PCD_ReadRegister(MFRC522::VersionReg);
  haveRfid = rfidVersion != 0x00 && rfidVersion != 0xFF;
  Serial.printf("RC522: version 0x%02X %s\n", rfidVersion, haveRfid ? "OK" : "(not found: check wiring, 3.3V, soldering)");

  flap.setPeriodHertz(50);
  flap.attach(PIN_SERVO, 500, 2400);
  flap.write(0);

  beep(100, 2000);
  blink(1);
  event("ready: touch a part");
  Serial.println("Commands: b = buzzer, l = LED, s = servo");
}

void loop() {
  if (millis() - lastMeasureAt > 250) {
    lastMeasureAt = millis();
    distanceCm = readDistanceCm();
  }

  int ir = digitalRead(PIN_IR);
  if (ir == LOW && lastIr == HIGH) { event("Bottle sensor: DETECTED"); beep(40, 3000); }
  lastIr = ir;

  int btn = digitalRead(PIN_RESIDUAL_BTN);
  if (btn == LOW && lastBtn == HIGH) { lastBtn = btn; draw(); openFlap(); }
  lastBtn = digitalRead(PIN_RESIDUAL_BTN);

  checkCard();

  if (Serial.available()) {
    char c = tolower(Serial.read());
    if (c == 'b') { event("Buzzer: 3 tones"); beep(200, 800); delay(80); beep(200, 1800); delay(80); beep(200, 2600); }
    else if (c == 'l') { event("LED: 5 blinks"); blink(5); }
    else if (c == 's') openFlap();
  }

  if (millis() - lastDrawAt > 200) {
    lastDrawAt = millis();
    draw();
  }
}
