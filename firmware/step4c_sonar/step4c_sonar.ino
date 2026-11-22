// TapTapon step 4c: ESP32 + HC-SR04 ultrasonic sensor only.
//
// What you should see:
//   Board's blue LED ON  = something closer than 20 cm (move your hand in front of the sensor)
//   Board's blue LED OFF = nothing near
//   Serial Monitor (115200): the distance in cm, 3 times a second
//   LED never turns on and Serial says "no echo": check TRIG/ECHO and the divider.
//
// Wiring:
//   HC-SR04 VCC -> VIN (5 V), GND -> GND, TRIG -> D26
//   ECHO -> 1k resistor -> breadboard row X;  row X -> D27;  row X -> 1k -> 1k -> GND
//   (the resistors turn the sensor's 5 V echo into a safe ~3.3 V for the ESP32)
#include <Arduino.h>

const int PIN_TRIG = 26, PIN_ECHO = 27, PIN_ONBOARD_LED = 2;
const float NEAR_CM = 20;

float readDistanceCm() {
  digitalWrite(PIN_TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH); delayMicroseconds(10);   // 10 us pulse starts a measurement
  digitalWrite(PIN_TRIG, LOW);
  unsigned long us = pulseIn(PIN_ECHO, HIGH, 30000);     // echo time; 30 ms timeout ~ 5 m
  return us ? us / 58.0 : -1;                            // ~58 us per cm (there and back)
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_ONBOARD_LED, OUTPUT);
}

void loop() {
  float d = readDistanceCm();
  digitalWrite(PIN_ONBOARD_LED, d > 0 && d < NEAR_CM ? HIGH : LOW);
  if (d < 0) Serial.println("no echo");
  else Serial.printf("%.1f cm\n", d);
  delay(300);
}
