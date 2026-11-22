// TapTapon step 2: OLED (already working) + external LED.
// The screen says what the LED should be doing, so you can compare.
//
// What you should see:
//   Screen says "LED: ON"  -> your LED (and the board's small blue LED) are lit
//   Screen says "LED: OFF" -> both are dark
//   It switches every second.
// Board LED blinks but yours doesn't? Your LED is backwards or the resistor/GND row is wrong.
//
// Wiring: OLED as in step 1 (VCC 3V3, GND, SCL D22, SDA D21)
//         D2 -> 220 ohm -> LED LONG leg; LED SHORT leg -> GND
// Library: Adafruit SSD1306
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const int PIN_LED = 2;  // the bin's status LED; also the board's onboard blue LED
Adafruit_SSD1306 oled(128, 64, &Wire, -1);
bool ledOn = false;

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED, OUTPUT);
  Wire.begin(21, 22);
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
}

void loop() {
  ledOn = !ledOn;
  digitalWrite(PIN_LED, ledOn ? HIGH : LOW);

  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.println("STEP 2: LED test");
  oled.setTextSize(3);
  oled.setCursor(0, 22);
  oled.println(ledOn ? "LED: ON" : "LED:OFF");
  oled.setTextSize(1);
  oled.setCursor(0, 56);
  oled.println("Compare with your LED");
  oled.display();

  Serial.println(ledOn ? "LED ON" : "LED OFF");
  delay(1000);
}
