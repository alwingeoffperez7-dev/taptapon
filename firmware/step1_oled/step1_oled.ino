// TapTapon step 1: OLED screen only. Nothing else connected.
// Once this works, the screen becomes the display for testing every other part.
//
// What you should see:
//   Screen: "STEP 1 OK" and a counter going up every second
//   Board:  the small blue onboard LED blinks once a second (proves the program is running)
//   Serial Monitor (115200): which I2C address the screen answered on
//
// Wiring: OLED VCC->3V3, GND->GND, SCL->D22, SDA->D21
// Library: Adafruit SSD1306 (say yes to Adafruit GFX)
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const int PIN_ONBOARD_LED = 2;
const int PIN_OLED_SDA = 21, PIN_OLED_SCL = 22;

Adafruit_SSD1306 oled(128, 64, &Wire, -1);
bool screenOk = false;
int seconds = 0;

void setup() {
  Serial.begin(115200);
  delay(300);
  pinMode(PIN_ONBOARD_LED, OUTPUT);
  Serial.println("\nStep 1: OLED only");

  // Ask every I2C address who's there, so a wiring or address problem is obvious.
  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
  byte found = 0;
  for (byte a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      Serial.printf("I2C device answered at 0x%02X\n", a);
      if (!found) found = a;
    }
  }
  if (!found) {
    Serial.println("Nothing answered on I2C. Check: VCC on 3V3, GND, SDA on D21, SCL on D22 (try swapping SDA/SCL).");
    return;
  }

  // Most 0.96" screens use 0x3C; some use 0x3D. Use whichever answered.
  screenOk = oled.begin(SSD1306_SWITCHCAPVCC, found);
  Serial.println(screenOk ? "OLED started" : "OLED answered but would not start (out of memory?)");
}

void loop() {
  digitalWrite(PIN_ONBOARD_LED, seconds % 2);  // blink once a second: the program is alive
  if (screenOk) {
    oled.clearDisplay();
    oled.setTextColor(SSD1306_WHITE);
    oled.setTextSize(2);
    oled.setCursor(0, 0);
    oled.println("STEP 1 OK");
    oled.setTextSize(1);
    oled.setCursor(0, 28);
    oled.println("TapTapon screen test");
    oled.setTextSize(2);
    oled.setCursor(0, 44);
    oled.printf("%d s", seconds);
    oled.display();
  }
  Serial.printf("Running %d s, screen %s\n", seconds, screenOk ? "OK" : "NOT FOUND");
  seconds++;
  delay(1000);
}
