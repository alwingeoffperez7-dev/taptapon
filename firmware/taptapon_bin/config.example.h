// Copy this file to config.h and fill it in. config.h is git-ignored: never commit it.
#pragma once

// Wi-Fi. In Wokwi use "Wokwi-GUEST", no password, channel 6. On real hardware use channel 0 (auto).
#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""
#define WIFI_CHANNEL 6

// Firebase: same public values as dashboard/js/firebase-config.js.
#define FIREBASE_API_KEY "AIzaSyAqyVsTLL5wjkgMb4FPrQoZE2EoxO6Xk7k"
#define FIREBASE_DB_URL "https://taptapon-default-rtdb.asia-southeast1.firebasedatabase.app"

// The bin's own login: the "device" account from README step 6. This one IS secret.
#define DEVICE_EMAIL "device@taptapon.test"
#define DEVICE_PASSWORD "PUT-THE-DEVICE-PASSWORD-HERE"

// Which bin this ESP32 is (must already exist under /bins).
#define BIN_ID "bin-01"

// 1 = no RC522: type a card UID in the Serial Monitor to "tap" (for Wokwi). 0 = real RC522 reader.
#define FAKE_RFID 1

// HX711 raw units per gram. Wokwi's 5 kg cell reads 0..2100 raw for 0..5000 g, so 2100 / 5000 = 0.42.
// For a real load cell: put a known weight on it, read the raw value, and divide by the grams.
#define PET_CALIBRATION 0.42
#define RESIDUAL_CALIBRATION 0.42
