// TapTapon WasteWise Bin: ESP32 firmware.
// Pins follow CLAUDE.md §2; every write follows the data contract in §3.
// The bin only RECORDS what happened (fill readings, deposits). It never awards points; the service does.
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ESP32Servo.h>
#include <HX711.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"
#if !FAKE_RFID
#include <SPI.h>
#include <MFRC522.h>
#endif

// ---- Pins (CLAUDE.md §2) ----
const int PIN_TRIG = 26, PIN_ECHO = 27;        // HC-SR04 (ECHO through the 1k/2k divider)
const int PIN_SERVO = 13;                      // residual flap
const int PIN_IR = 32;                         // break-beam receiver: LOW = beam broken
const int PIN_HX1_DT = 16, PIN_HX1_SCK = 17;   // load cell #1: recyclables (PET)
const int PIN_HX2_DT = 4, PIN_HX2_SCK = 25;    // load cell #2: residual
const int PIN_LED = 2, PIN_BUZZER = 33;     // GPIO 2 = also the board's onboard LED
const int PIN_RESIDUAL_BTN = 14;               // press to open the residual flap
const int PIN_RC522_SS = 5, PIN_RC522_RST = 15;
const int PIN_OLED_SDA = 21, PIN_OLED_SCL = 22;  // front screen (optional), ESP32 default I2C pins

// ---- Timing ----
const unsigned long MEASURE_EVERY_MS = 2000;
const unsigned long HEARTBEAT_MS = 30000;          // dashboard shows OFFLINE after 2 min of silence
const unsigned long DEPOSIT_SETTLE_MS = 1500;      // weigh only after the last bottle has dropped
const unsigned long TAP_VALID_MS = 30000;          // a tap counts for deposits in the next 30 s
const unsigned long FLAP_OPEN_MS = 5000;
const unsigned long TOKEN_LIFETIME_MS = 50UL * 60 * 1000;  // Firebase ID tokens last 60 min; renew at 50

// Read from /config at boot, so admins can change them without reflashing.
float binDepthCm = 45, fullThreshold = 80, minDepositGrams = 10;

HX711 scalePet, scaleResidual;
Servo flap;
#if !FAKE_RFID
MFRC522 rfid(PIN_RC522_SS, PIN_RC522_RST);
#endif

String idToken;
unsigned long tokenAt = 0;
String tappedUid;
unsigned long tappedAt = 0;
int pendingBottles = 0;
unsigned long lastBeamAt = 0;
// Beam timing (anti-cheat): a dropped bottle falls through the beam in a fraction of a second,
// while a hand waved or held in the port blocks it much longer. Only short blocks count.
const unsigned long BEAM_MIN_MS = 5;     // shorter = electrical glitch, ignored
const unsigned long BEAM_MAX_MS = 500;   // longer = hand / stuck object, rejected
// Set by the interrupt below, read by checkBottles(). volatile = may change at any moment.
volatile unsigned long beamBlockedAt = 0;
volatile int beamValid = 0, beamRejected = 0;
volatile unsigned long lastRejectedMs = 0;

// Runs the instant the beam changes, even while the bin is busy uploading to Firebase,
// so a fast-falling bottle can never slip through unseen.
void IRAM_ATTR onBeamChange() {
  unsigned long now = millis();
  if (digitalRead(PIN_IR) == LOW) {          // beam just got blocked
    beamBlockedAt = now;
  } else if (beamBlockedAt) {                // beam just cleared: how long was it blocked?
    unsigned long blocked = now - beamBlockedAt;
    beamBlockedAt = 0;
    if (blocked < BEAM_MIN_MS) return;       // glitch
    if (blocked <= BEAM_MAX_MS) beamValid++;
    else { beamRejected++; lastRejectedMs = blocked; }
  }
}
float petBaseline = 0, residualBaseline = 0;
unsigned long lastMeasureAt = 0, lastReportAt = 0;
int lastReportedFill = -100;
bool binFull = false;

// ---------------------------------------------------------------- Front screen (optional OLED)
// A 0.96" SSD1306 on the front panel tells the student what to do and what they earned.
// If it isn't connected, every screen call simply does nothing.
Adafruit_SSD1306 oled(128, 64, &Wire, -1);
bool haveOled = false;
int shownFill = -1;                // fill % shown on the idle screen
unsigned long messageUntil = 0;    // while a message is showing, the idle screen waits
bool idleShown = false;

// One screen: small top line, big middle text (max 10 characters per line, \n for a 2nd line),
// small bottom line (max 21 characters). holdMs = how long before the idle screen returns.
void screen(const char* top, const char* big, const char* bottom, unsigned long holdMs) {
  if (!haveOled) return;
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1); oled.setCursor(0, 0);  oled.print(top);
  oled.setTextSize(2); oled.setCursor(0, 18); oled.print(big);
  oled.setTextSize(1); oled.setCursor(0, 55); oled.print(bottom);
  oled.display();
  messageUntil = millis() + holdMs;
  idleShown = false;
}

// The resting screen between students: what to do, and how full the bin is.
void showIdle() {
  if (!haveOled || idleShown || millis() < messageUntil) return;
  char bottom[24];
  snprintf(bottom, sizeof bottom, "Bin %d%% full", max(shownFill, 0));
  if (binFull) screen("TapTapon", "BIN FULL", "Use another bin", 0);
  else screen("TapTapon   I-Tap!", "Tap your\nID card", bottom, 0);
  idleShown = true;
}

// The service's verdict for a tapped deposit, in words a student understands.
void showVerdictScreen(const String& reason) {
  if (reason == "ok")                screen("Salamat!", "+1 POINT!", "Keep segregating!", 5000);
  else if (reason == "daily_cap")    screen("Thank you!", "Daily max\nreached", "Come back tomorrow", 5000);
  else if (reason == "unknown_tag")  screen("Card not known", "Card not\nregistered", "Register at the booth", 5000);
  else if (reason == "not_verified") screen("Hmm...", "Not\nverified", "Drop a real bottle", 5000);
  else if (reason == "residual")     screen("Salamat!", "Thanks!", "Residual: no points", 5000);
  else                               screen("Result", reason.c_str(), "", 5000);
}

// ---------------------------------------------------------------- HTTP + Firebase REST

// ponytail: setInsecure() skips TLS certificate checks. OK for the prototype;
// pin Google's root CA with client.setCACert() before a real deployment.
int request(const char* method, const String& url, const String& body, String& response) {
  int code = 0;
  for (int attempt = 0; attempt < 2; attempt++) {  // one retry: Wi-Fi hiccups are common
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");
    code = http.sendRequest(method, body);
    response = code > 0 ? http.getString() : http.errorToString(code);
    http.end();
    if (code > 0) break;  // negative = connection error; HTTP errors (4xx/5xx) aren't retried
    delay(500);
  }
  return code;
}

// Tiny readers for the few flat JSON replies we need, so no JSON library is required.
String jsonString(const String& json, const char* key) {
  int i = json.indexOf(String("\"") + key + "\"");
  if (i < 0) return "";
  i = json.indexOf('"', json.indexOf(':', i) + 1);
  return i < 0 ? "" : json.substring(i + 1, json.indexOf('"', i + 1));
}
float jsonNumber(const String& json, const char* key, float fallback) {
  int i = json.indexOf(String("\"") + key + "\"");
  return i < 0 ? fallback : json.substring(json.indexOf(':', i) + 1).toFloat();
}

// Signs in as the device account (same as the simulator) to get a token the security rules accept.
bool signIn() {
  String body = String("{\"email\":\"") + DEVICE_EMAIL + "\",\"password\":\"" + DEVICE_PASSWORD +
                "\",\"returnSecureToken\":true}";
  String res;
  int code = request("POST", String("https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key=") +
                     FIREBASE_API_KEY, body, res);
  if (code != 200) {
    // Negative codes are network failures (couldn't reach Google); 400 means wrong email/password.
    if (code < 0) Serial.printf("Sign-in failed: network error (%s). Retrying in 10 s\n", res.c_str());
    else Serial.printf("Sign-in failed (HTTP %d). Check DEVICE_EMAIL / DEVICE_PASSWORD in config.h\n", code);
    return false;
  }
  idToken = jsonString(res, "idToken");
  tokenAt = millis();
  Serial.println("Signed in as the device account");
  return true;
}

// Database URL for a path, with a fresh token. Never print it: it contains the token.
String dbUrl(const String& path) {
  static unsigned long lastAttemptAt = 0;
  bool needToken = idToken.isEmpty() || millis() - tokenAt > TOKEN_LIFETIME_MS;
  // Retry at most every 10 s, so a network outage doesn't block the bin on sign-in attempts.
  if (needToken && (lastAttemptAt == 0 || millis() - lastAttemptAt > 10000)) {
    lastAttemptAt = millis();
    signIn();
  }
  return String(FIREBASE_DB_URL) + path + ".json?auth=" + idToken;
}

void loadConfig() {
  String res;
  if (request("GET", String(FIREBASE_DB_URL) + "/config.json", "", res) != 200) {
    Serial.println("Couldn't read /config, using defaults");
    return;
  }
  binDepthCm = jsonNumber(res, "binDepthCm", binDepthCm);
  fullThreshold = jsonNumber(res, "fullThresholdPercent", fullThreshold);
  minDepositGrams = jsonNumber(res, "minDepositGrams", minDepositGrams);
  Serial.printf("Config: depth %.0f cm, full at %.0f%%, min deposit %.0f g\n", binDepthCm, fullThreshold, minDepositGrams);
}

// ---------------------------------------------------------------- Feedback (LED + buzzer)

// Square wave by hand, so the buzzer never fights the servo for the ESP32's PWM timers.
void beep(int ms, int hz = 2000) {
  long halfUs = 500000L / hz;
  for (long t = 0; t < ms * 1000L; t += halfUs * 2) {
    digitalWrite(PIN_BUZZER, HIGH); delayMicroseconds(halfUs);
    digitalWrite(PIN_BUZZER, LOW);  delayMicroseconds(halfUs);
  }
}
void showFullLed() { digitalWrite(PIN_LED, binFull ? HIGH : LOW); }  // solid LED = bin full
void celebrate() {                                                   // fast blinks = points earned
  for (int i = 0; i < 3; i++) { digitalWrite(PIN_LED, HIGH); beep(70, 2600); digitalWrite(PIN_LED, LOW); delay(70); }
  showFullLed();
}
void grumble() { beep(150, 400); delay(80); beep(150, 400); }

// ---------------------------------------------------------------- Fill level

float readDistanceCm() {
  float r[5];
  for (int i = 0; i < 5; i++) {
    digitalWrite(PIN_TRIG, LOW);  delayMicroseconds(2);
    digitalWrite(PIN_TRIG, HIGH); delayMicroseconds(10);
    digitalWrite(PIN_TRIG, LOW);
    unsigned long us = pulseIn(PIN_ECHO, HIGH, 30000);  // 30 ms timeout ≈ 5 m
    r[i] = us ? us / 58.0 : binDepthCm;                 // no echo = nothing in range = empty
    delay(15);
  }
  // Median of 5: one stray echo can't make the bin look full.
  for (int i = 1; i < 5; i++) for (int j = i; j > 0 && r[j - 1] > r[j]; j--) { float t = r[j]; r[j] = r[j - 1]; r[j - 1] = t; }
  return r[2];
}

void measureFill() {
  float d = readDistanceCm();
  int fill = constrain((int)round((binDepthCm - d) / binDepthCm * 100), 0, 100);  // CLAUDE.md §3 formula
  binFull = fill >= fullThreshold;
  showFullLed();
  if (fill != shownFill) { shownFill = fill; idleShown = false; }  // refresh the idle screen

  // Report on the heartbeat, or right away when the level really changed.
  if (millis() - lastReportAt < HEARTBEAT_MS && abs(fill - lastReportedFill) < 2) return;
  String body = String("{\"fillPercent\":") + fill + ",\"distanceCm\":" + String(max(0.0f, d), 1) +
                ",\"lastSeen\":{\".sv\":\"timestamp\"}}";
  String res;
  int code = request("PATCH", dbUrl("/bins/" BIN_ID), body, res);
  if (code == 200) {
    if (fill != lastReportedFill) Serial.printf("Fill %d%% (%.1f cm)\n", fill, d);
    lastReportedFill = fill;
    lastReportAt = millis();
  } else {
    Serial.printf("Fill report failed (HTTP %d): %s\n", code, res.c_str());
  }
}

// ---------------------------------------------------------------- Deposits

// Asks the database what the service decided, so the student gets feedback at the bin.
// ponytail: blocks up to 6 s; bottles dropped during that wait are counted afterwards.
void showVerdict(const String& key) {
  for (int i = 0; i < 6; i++) {
    delay(1000);
    String res;
    if (request("GET", dbUrl(String("/deposits/") + key + "/reason"), "", res) != 200 || res == "null") continue;
    res.replace("\"", "");
    Serial.printf("  service: %s\n", res.c_str());
    showVerdictScreen(res);
    if (res == "ok") celebrate(); else grumble();
    return;
  }
  Serial.println("  service didn't answer (is it running?)");
  screen("Saved!", "Bottle\nsaved", "Points appear soon", 4000);
}

// Weight from the median of 7 readings: one misread from electrical noise can't turn an 18 g
// bottle into a huge number (an average would be thrown off by a single spike).
long medianRaw(HX711& scale) {
  long r[7];
  for (int i = 0; i < 7; i++) r[i] = scale.read();
  for (int i = 1; i < 7; i++) for (int j = i; j > 0 && r[j - 1] > r[j]; j--) { long t = r[j]; r[j] = r[j - 1]; r[j - 1] = t; }
  return r[3];
}
float medianGrams(HX711& scale) { return (medianRaw(scale) - scale.get_offset()) / scale.get_scale(); }

// A scale that isn't connected would make read() wait forever and freeze the bin.
// Missing scales read as 0 g, so deposits still go through (prototype v1 runs without scales).
bool petScaleOk = false, residualScaleOk = false;
float petGrams() { return petScaleOk ? medianGrams(scalePet) : 0; }
float residualGrams() { return residualScaleOk ? medianGrams(scaleResidual) : 0; }

// Returns false if the deposit didn't reach Firebase, so the caller can keep it and retry.
bool sendDeposit(const char* type, int count, float grams) {
  bool tapped = tappedUid.length() && millis() - tappedAt < TAP_VALID_MS;
  String body = String("{\"binId\":\"" BIN_ID "\",\"type\":\"") + type + "\",\"count\":" + min(count, 10) +
                ",\"weightGrams\":" + (int)round(max(0.0f, grams)) + ",\"timestamp\":{\".sv\":\"timestamp\"}";
  if (tapped) body += ",\"tagUid\":\"" + tappedUid + "\"";
  body += "}";

  String res;
  int code = request("POST", dbUrl("/deposits"), body, res);
  if (code != 200) {
    Serial.printf("Deposit failed (HTTP %d): %s. Will retry.\n", code, res.c_str());
    grumble();
    return false;
  }
  String key = jsonString(res, "name");
  Serial.printf("Deposit %s: %s x%d, %.0f g, tag %s\n", key.c_str(), type, count, grams, tapped ? tappedUid.c_str() : "none");
  if (tapped) showVerdict(key);  // no tap = no points to celebrate
  else if (strcmp(type, "pet") == 0) screen("Bottle counted", "No tap =\nno point", "Tap ID before drop", 4000);
  return true;
}

// Counts bottles through the IR beam, then weighs them once they've all dropped.
void checkBottles() {
  // Take whatever the interrupt counted since last time.
  noInterrupts();
  int valid = beamValid, rejected = beamRejected;
  unsigned long rejectedMs = lastRejectedMs;
  beamValid = 0; beamRejected = 0;
  interrupts();

  if (valid > 0) {
    pendingBottles += valid;
    lastBeamAt = millis();
    beep(30, 3000);
    Serial.printf("Bottle counted (%d)\n", pendingBottles);
    char top[22];
    snprintf(top, sizeof top, "Bottles: %d", pendingBottles);
    screen(top, "Counted!", "Checking points...", 10000);
  }
  if (rejected > 0) {
    Serial.printf("Ignored: beam blocked %lu ms (longer than %lu ms, not a falling bottle)\n", rejectedMs, BEAM_MAX_MS);
    beep(150, 400);
    screen("Hmm...", "Bottles\nonly!", "Drop it, don't hold", 3000);
  }

  bool broken = digitalRead(PIN_IR) == LOW;
  if (pendingBottles > 0 && !broken && millis() - lastBeamAt > DEPOSIT_SETTLE_MS) {
    float now = petGrams();
    // Only move the baseline once the deposit is saved; otherwise its weight would be lost.
    if (sendDeposit("pet", pendingBottles, now - petBaseline)) {
      petBaseline = now;
      pendingBottles = 0;
    } else {
      lastBeamAt = millis();  // try again after another settle period
    }
  }
}

void checkResidualButton() {
  if (digitalRead(PIN_RESIDUAL_BTN) != LOW) return;
  Serial.println("Residual flap open");
  screen("Residual side", "Flap open", "Drop your trash now", FLAP_OPEN_MS + 1000);
  flap.write(90);
  delay(FLAP_OPEN_MS);
  flap.write(0);
  delay(1000);  // let the load cell settle
  float now = residualGrams();
  float added = now - residualBaseline;
  if (added < 2) { Serial.println("Nothing dropped"); return; }  // ignore scale noise
  if (sendDeposit("residual", 0, added)) residualBaseline = now;  // on failure, the weight joins the next residual deposit
}

// ---------------------------------------------------------------- Card tap

void checkTap() {
  String uid;
#if FAKE_RFID
  // Wokwi has no RC522: type a UID like A1B2C3D4 in the Serial Monitor and press Enter.
  if (!Serial.available()) return;
  uid = Serial.readStringUntil('\n');
#else
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  rfid.PICC_HaltA();
#endif
  uid.trim();
  // Keep only hex digits, so "a1 b2:c3 d4" (or a pasted `A1B2C3D4`) becomes A1B2C3D4,
  // then accept only real MIFARE UID lengths: 4 bytes (8 chars) or 7 bytes (14 chars).
  String clean;
  for (unsigned int i = 0; i < uid.length(); i++) if (isxdigit(uid[i])) clean += (char)toupper(uid[i]);
  if (clean.length() != 8 && clean.length() != 14) {
    if (uid.length()) Serial.printf("Not a card UID: %s (type 8 or 14 hex characters, e.g. A1B2C3D4)\n", uid.c_str());
    return;
  }
  uid = clean;
  tappedUid = uid;
  tappedAt = millis();
  Serial.printf("Card tapped: %s (register unknown cards on the admin page)\n", uid.c_str());
  beep(80, 1800);
  char top[22];
  snprintf(top, sizeof top, "Card %s", uid.c_str());
  screen(top, "Drop a\nbottle", "within 30 seconds", TAP_VALID_MS);
}

// ---------------------------------------------------------------- Setup / loop

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(50);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_IR, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_IR), onBeamChange, CHANGE);
  pinMode(PIN_RESIDUAL_BTN, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  flap.setPeriodHertz(50);
  flap.attach(PIN_SERVO, 500, 2400);
  flap.write(0);

  // Both bins must be empty at power-on: the current weight becomes zero (median, spike-proof).
  // A scale that doesn't answer within 1 s is treated as not connected.
  scalePet.begin(PIN_HX1_DT, PIN_HX1_SCK);
  petScaleOk = scalePet.wait_ready_timeout(1000);
  if (petScaleOk) { scalePet.set_scale(PET_CALIBRATION); scalePet.set_offset(medianRaw(scalePet)); }
  scaleResidual.begin(PIN_HX2_DT, PIN_HX2_SCK);
  residualScaleOk = scaleResidual.wait_ready_timeout(1000);
  if (residualScaleOk) { scaleResidual.set_scale(RESIDUAL_CALIBRATION); scaleResidual.set_offset(medianRaw(scaleResidual)); }
  Serial.printf("PET scale: %s, residual scale: %s\n",
                petScaleOk ? "OK" : "NOT CONNECTED (weights read 0 g)",
                residualScaleOk ? "OK" : "NOT CONNECTED (weights read 0 g)");

#if !FAKE_RFID
  SPI.begin();
  rfid.PCD_Init();
#endif

  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
  haveOled = oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  Serial.println(haveOled ? "Front screen: OK" : "Front screen: not connected (that's fine)");
  screen("TapTapon", "Starting", "Connecting Wi-Fi...", 0);

  Serial.printf("Connecting to %s", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
  while (WiFi.status() != WL_CONNECTED) { delay(250); Serial.print("."); }
  Serial.println(" connected");

  loadConfig();
  signIn();
  Serial.println(FAKE_RFID ? "Ready. Type a card UID (e.g. A1B2C3D4) to tap." : "Ready. Tap a card.");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) WiFi.reconnect();  // events are skipped while offline
  checkTap();
  showIdle();
  checkBottles();
  checkResidualButton();
  if (millis() - lastMeasureAt > MEASURE_EVERY_MS) {
    lastMeasureAt = millis();
    measureFill();
    // Print the PET scale when it changes, to check the load cell and its calibration.
    static float lastShownGrams = 0;
    float grams = petGrams() - petBaseline;
    if (petScaleOk && fabs(grams - lastShownGrams) >= 2) {
      Serial.printf("PET scale: %+.0f g since last deposit\n", grams);
      lastShownGrams = grams;
    }
  }
}
