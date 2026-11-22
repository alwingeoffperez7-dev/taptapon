// TapTapon step 6 DIAGNOSTIC: shows the HX711's raw numbers with no zeroing or filtering,
// and says what they mean. Same wiring as step 6 (DT -> D16, SCK -> D17, VCC -> 3V3, GND -> GND).
//
// Leave it running ~10 s untouched, then press the free end of the bar hard for ~5 s.
// Copy ~20 lines from the Serial Monitor (115200) and send them.
//
// Reading the raw value (the HX711 gives a number from -8388608 to 8388607):
//   stuck at  8388607 or -8388608        -> SATURATED: load cell circuit open or shorted (a wire/joint)
//   stuck at exactly the same value       -> chip not converting (power / DT / SCK problem)
//   changes a little (tens..hundreds) and jumps by thousands when pressed -> WORKING
// Library: HX711 (by Bogdan Necula)
#include <HX711.h>

const int PIN_DT = 16, PIN_SCK = 17;
HX711 scale;
long first = 0;
bool haveFirst = false;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\nStep 6 DIAGNOSTIC: raw HX711 numbers");
  scale.begin(PIN_DT, PIN_SCK);
  if (!scale.wait_ready_timeout(2000)) {
    Serial.println("HX711 NOT FOUND: DT/SCK/VCC/GND wiring or header joints.");
  }
}

void loop() {
  if (!scale.wait_ready_timeout(1000)) {
    Serial.println("HX711 not ready (no data): check VCC, GND, DT, SCK");
    return;
  }
  long raw = scale.read();
  if (!haveFirst) { first = raw; haveFirst = true; }

  const char* verdict;
  if (raw >= 8388000 || raw <= -8388000) verdict = "SATURATED -> load cell circuit open or shorted";
  else verdict = "in range";
  Serial.printf("raw %9ld   change since start %8ld   %s\n", raw, raw - first, verdict);
  delay(250);
}
