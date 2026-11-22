// TapTapon step 4b: ESP32 + SG90 servo only.
//
// What you should see: the servo arm swings 0 -> 90 -> 0 degrees every 2 seconds
// (90 = residual flap open, 0 = closed). The board's blue LED is on while the flap is "open".
// If the ESP32 keeps restarting (blue LED flicker, nothing moves), USB can't supply enough
// current for the servo: power the servo's red wire from your 5 V supply instead, with its
// GND connected to the ESP32 GND too.
//
// Wiring: servo orange (signal) -> D13, red (+) -> VIN, brown (-) -> GND
// Library: ESP32Servo
#include <ESP32Servo.h>

const int PIN_SERVO = 13, PIN_ONBOARD_LED = 2;
Servo flap;

void setup() {
  Serial.begin(115200);
  pinMode(PIN_ONBOARD_LED, OUTPUT);
  flap.setPeriodHertz(50);           // standard hobby-servo signal
  flap.attach(PIN_SERVO, 500, 2400); // SG90 pulse range in microseconds
  flap.write(0);
}

void loop() {
  flap.write(90); digitalWrite(PIN_ONBOARD_LED, HIGH); Serial.println("open (90)");
  delay(2000);
  flap.write(0);  digitalWrite(PIN_ONBOARD_LED, LOW);  Serial.println("closed (0)");
  delay(2000);
}
