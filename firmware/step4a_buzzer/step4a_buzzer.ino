// TapTapon step 4a: ESP32 + passive buzzer only.
//
// What you should hear: three rising tones every 3 seconds (low, mid, high).
// The board's small blue LED flashes with each set, so you know the program is running.
// LED flashes but no sound? Check the buzzer's + and - legs, or you may have the ACTIVE buzzer
// (sealed bottom, sticker on top): that one only clicks or gives a single tone with this code.
//
// Wiring: buzzer + leg -> D33, buzzer - leg -> GND   (the + is usually marked on the top)
#include <Arduino.h>

const int PIN_BUZZER = 33, PIN_ONBOARD_LED = 2;

// Square wave by hand (same as the real bin), so it never fights the servo for PWM timers later.
void beep(int ms, int hz) {
  long halfUs = 500000L / hz;
  for (long t = 0; t < ms * 1000L; t += halfUs * 2) {
    digitalWrite(PIN_BUZZER, HIGH); delayMicroseconds(halfUs);
    digitalWrite(PIN_BUZZER, LOW);  delayMicroseconds(halfUs);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_ONBOARD_LED, OUTPUT);
}

void loop() {
  digitalWrite(PIN_ONBOARD_LED, HIGH);
  beep(200, 800);  delay(80);
  beep(200, 1800); delay(80);
  beep(200, 2600);
  digitalWrite(PIN_ONBOARD_LED, LOW);
  Serial.println("beep beep beep");
  delay(3000);
}
