// TapTapon step 4b (diagnostic): drive the SG90 WITHOUT any servo library.
// A hobby servo is steered by a pulse every 20 ms: ~1.0 ms wide = one end, ~2.0 ms = the other end.
// This sketch makes those pulses by hand, so if the servo moves here but not with step4b_servo,
// the library was the problem; if it still only hums, the servo (or its power) is the problem.
//
// What you should see: the arm goes to one side for 2 s, then the other side for 2 s, repeating.
// The board's blue LED is on during the "open" side.
//
// Wiring (same as before): orange -> D13, red -> VIN (or 5 V supply), brown -> GND
#include <Arduino.h>

const int PIN_SERVO = 13, PIN_ONBOARD_LED = 2;

// Hold the servo at one position for `ms` milliseconds by sending one pulse every 20 ms.
void holdPulse(int pulseUs, int ms) {
  for (int t = 0; t < ms; t += 20) {
    digitalWrite(PIN_SERVO, HIGH);
    delayMicroseconds(pulseUs);
    digitalWrite(PIN_SERVO, LOW);
    delayMicroseconds(20000 - pulseUs);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_SERVO, OUTPUT);
  pinMode(PIN_ONBOARD_LED, OUTPUT);
}

void loop() {
  digitalWrite(PIN_ONBOARD_LED, HIGH);
  Serial.println("pulse 2.0 ms (one side)");
  holdPulse(2000, 2000);
  digitalWrite(PIN_ONBOARD_LED, LOW);
  Serial.println("pulse 1.0 ms (other side)");
  holdPulse(1000, 2000);
}
