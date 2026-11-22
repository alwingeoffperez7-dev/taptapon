// TapTapon step 6: ESP32 + ONE load cell with its HX711 (the PET / bottle scale).
// Also finds the calibration number the real bin needs (PET_CALIBRATION in config.h).
//
// How to use (Serial Monitor, 115200 baud, "Newline" line ending):
//   1. Power on with NOTHING on the load cell. It zeroes itself ("tare").
//   2. Watch "raw" change when you press gently on the free end of the bar.
//   3. Put something of known weight on it (e.g. a full 500 mL water bottle, weigh it on a
//      kitchen scale first), type its weight in grams, e.g. 520, and press Enter.
//   4. It prints "PET_CALIBRATION = 21.37" (your number) and from then on shows grams.
//      Copy that number into firmware/taptapon_bin/config.h.
//   Type t + Enter to zero again (empty the scale first).
// The board's blue LED turns on when more than 10 g is on the scale (the bin's minimum deposit).
//
// Wiring:
//   Load cell wires -> HX711: red E+, black E-, white A-, green A+  (if weight reads negative, swap white/green)
//   HX711 -> ESP32: VCC -> 3V3, GND -> GND, DT -> D16 (RX2), SCK -> D17 (TX2)
// Library: HX711 (by Bogdan Necula)
#include <HX711.h>

const int PIN_DT = 16, PIN_SCK = 17, PIN_ONBOARD_LED = 2;
HX711 scale;
float calibration = 1.0;   // 1.0 = show raw units until you calibrate
bool calibrated = false;

// Median of 7 raw readings minus the zero point. A single misread (a wild spike from a loose
// wire or electrical noise) lands at the top or bottom of the sorted list and is ignored,
// whereas an average would be ruined by it.
long medianRaw() {
  long r[7];
  for (int i = 0; i < 7; i++) r[i] = scale.read();
  for (int i = 1; i < 7; i++) for (int j = i; j > 0 && r[j - 1] > r[j]; j--) { long t = r[j]; r[j] = r[j - 1]; r[j - 1] = t; }
  return r[3];
}
float medianValue() { return medianRaw() - scale.get_offset(); }

// Zero the scale with the median too: an averaged tare is thrown off by a single spike.
void zero() { scale.set_offset(medianRaw()); }

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(100);
  pinMode(PIN_ONBOARD_LED, OUTPUT);
  delay(300);
  Serial.println("\nStep 6: scale test (spike filter v2)");

  scale.begin(PIN_DT, PIN_SCK);
  if (!scale.wait_ready_timeout(2000)) {
    Serial.println("HX711 NOT FOUND: check DT->D16, SCK->D17, VCC->3V3, GND, and the solder joints.");
    return;
  }
  scale.set_scale(calibration);
  zero();   // median of 7 readings as "zero", so one spike cannot shift it
  Serial.println("Zeroed. Press the bar gently and watch the numbers.");
  Serial.println("To calibrate: put a known weight on it, type its grams (e.g. 520) and press Enter.");
}

void loop() {
  if (!scale.is_ready()) { delay(50); return; }

  if (Serial.available()) {
    String in = Serial.readStringUntil('\n');
    in.trim();
    if (in == "t") {
      zero();
      Serial.println("Zeroed again.");
    } else if (in.toFloat() > 0) {
      // calibration = raw units per gram: (reading with weight, in raw units) / (real grams)
      float grams = in.toFloat();
      float raw = medianValue();         // raw units above zero, spike-proof
      // Refuse to calibrate on an empty (or upside-down) scale: that is what gives tiny or negative numbers.
      if (raw < 5000) {
        Serial.printf("NOT CALIBRATED: the scale reads raw %.0f, which is basically empty.\n", raw);
        Serial.println("Hang the weight first, wait until raw is steady and big (e.g. 300000), THEN type its grams.");
        Serial.println("(If raw goes NEGATIVE with the weight on, flip the load cell over.)");
        return;
      }
      calibration = raw / grams;
      scale.set_scale(calibration);
      calibrated = true;
      Serial.printf("PET_CALIBRATION = %.2f   <- copy this into config.h\n", calibration);
    }
  }

  float value = medianValue() / calibration;
  digitalWrite(PIN_ONBOARD_LED, calibrated && value > 10 ? HIGH : LOW);
  if (calibrated) Serial.printf("%.0f g\n", value);
  else Serial.printf("raw %.0f\n", value);
  delay(300);
}
