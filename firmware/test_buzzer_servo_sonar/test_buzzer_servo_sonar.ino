// TapTapon test 2: buzzer + servo + HC-SR04 ultrasonic. No Wi-Fi, no screen needed.
// Same pins as the real bin (CLAUDE.md §2).
//
// What you should see:
//   Power on                     -> 3 rising beeps, servo moves to 0 (closed)
//   Distance printed every 0.3 s in the Serial Monitor (115200)
//   Hand closer than 10 cm       -> beep + servo opens to 90
//   Hand taken away              -> low beep + servo closes
//   Type b + Enter               -> buzzer test; type s + Enter -> servo swing
// If the ESP32 restarts when the servo moves, USB power is too weak for the servo.
//
// Wiring (use the breadboard rails: VIN -> 5V rail, GND -> GND rail):
//   Buzzer (passive): + -> D33, - -> GND rail
//   Servo:  orange -> D13, red -> 5V rail, brown -> GND rail
//   HC-SR04: VCC -> 5V rail, GND -> GND rail, TRIG -> D26,
//            ECHO -> 1k -> row X, row X -> D27, row X -> 1k -> 1k -> GND rail
// Library: ESP32Servo
#include <ESP32Servo.h>

const int PIN_TRIG = 26, PIN_ECHO = 27;
const int PIN_SERVO = 13, PIN_BUZZER = 33;
const float OPEN_BELOW_CM = 10;

Servo flap;
bool isOpen = false;
unsigned long lastMeasureAt = 0;

// Square wave by hand, so the buzzer never fights the servo for the ESP32's PWM timers.
void beep(int ms, int hz) {
  long halfUs = 500000L / hz;
  for (long t = 0; t < ms * 1000L; t += halfUs * 2) {
    digitalWrite(PIN_BUZZER, HIGH); delayMicroseconds(halfUs);
    digitalWrite(PIN_BUZZER, LOW);  delayMicroseconds(halfUs);
  }
}

float readDistanceCm() {
  digitalWrite(PIN_TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  unsigned long us = pulseIn(PIN_ECHO, HIGH, 30000);  // 30 ms timeout ~ 5 m
  return us ? us / 58.0 : -1;                         // -1 = no echo
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\nTest 2: buzzer + servo + HC-SR04");
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  flap.setPeriodHertz(50);
  flap.attach(PIN_SERVO, 500, 2400);
  flap.write(0);

  beep(150, 1000); delay(60); beep(150, 1800); delay(60); beep(150, 2600);
  Serial.println("Move your hand toward the sensor. Commands: b = buzzer, s = servo");
}

void loop() {
  if (millis() - lastMeasureAt > 300) {
    lastMeasureAt = millis();
    float d = readDistanceCm();
    if (d < 0) Serial.println("Distance: no echo (check TRIG/ECHO wiring and the divider)");
    else Serial.printf("Distance: %.1f cm\n", d);

    bool near = d > 0 && d < OPEN_BELOW_CM;
    if (near && !isOpen)  { beep(80, 2600); flap.write(90); isOpen = true;  Serial.println("  -> servo OPEN"); }
    if (!near && isOpen)  { beep(80, 900);  flap.write(0);  isOpen = false; Serial.println("  -> servo CLOSED"); }
  }

  if (Serial.available()) {
    char c = tolower(Serial.read());
    if (c == 'b') { Serial.println("Buzzer: low, mid, high"); beep(200, 800); delay(80); beep(200, 1800); delay(80); beep(200, 2600); }
    if (c == 's') { Serial.println("Servo: 0 -> 90 -> 0"); flap.write(90); delay(800); flap.write(0); delay(800); isOpen = false; }
  }
}
