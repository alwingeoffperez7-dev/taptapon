// TapTapon step 5: ESP32 + IR break-beam pair only (+ buzzer if it is still wired).
// Shows how long each object blocked the beam, and whether the real bin would COUNT it:
//   5..500 ms  = COUNTED (a bottle falling through)          -> short high beep
//   > 500 ms   = IGNORED (a hand waved slowly / held there)  -> low beep
// Drop a bottle through, then wave your hand slowly, and compare the times.
//
// Wiring (power BOTH from 3V3):
//   Emitter:  VCC -> 3V3, GND -> GND
//   Receiver: VCC -> 3V3, GND -> GND, signal -> D32
//   Optional: buzzer + -> D33, - -> GND
#include <Arduino.h>

const int PIN_IR = 32, PIN_BUZZER = 33, PIN_ONBOARD_LED = 2;
const unsigned long BEAM_MIN_MS = 5, BEAM_MAX_MS = 500;   // same limits as the real bin

int lastState = HIGH;
unsigned long blockedAt = 0;
int counted = 0, ignored = 0;

void beep(int ms, int hz) {
  long halfUs = 500000L / hz;
  for (long t = 0; t < ms * 1000L; t += halfUs * 2) {
    digitalWrite(PIN_BUZZER, HIGH); delayMicroseconds(halfUs);
    digitalWrite(PIN_BUZZER, LOW);  delayMicroseconds(halfUs);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_IR, INPUT_PULLUP);   // receiver pulls the line LOW when the beam is broken
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_ONBOARD_LED, OUTPUT);
  delay(300);
  Serial.printf("\nStep 5: break-beam timing. Beam is %s right now.\n",
                digitalRead(PIN_IR) == LOW ? "BROKEN (check aim)" : "clear");
}

void loop() {
  int state = digitalRead(PIN_IR);
  digitalWrite(PIN_ONBOARD_LED, state == LOW ? HIGH : LOW);

  if (state == LOW && lastState == HIGH) blockedAt = millis();          // beam just blocked
  if (state == HIGH && lastState == LOW) {                              // beam just cleared
    unsigned long blocked = millis() - blockedAt;
    if (blocked < BEAM_MIN_MS) {
      // glitch, ignore silently
    } else if (blocked <= BEAM_MAX_MS) {
      counted++;
      Serial.printf("blocked %4lu ms -> COUNTED   (counted %d, ignored %d)\n", blocked, counted, ignored);
      beep(40, 3000);
    } else {
      ignored++;
      Serial.printf("blocked %4lu ms -> IGNORED   (counted %d, ignored %d)\n", blocked, counted, ignored);
      beep(150, 400);
    }
  }
  lastState = state;
}
