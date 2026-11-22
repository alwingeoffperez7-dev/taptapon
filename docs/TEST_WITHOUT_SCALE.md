# Testing the real bin without scales, servo or capacitor

Use this to demo or test the full system (tap → bottle → points → phone, plus full-bin Telegram alert) while the load cells, servo and capacitor are not installed yet.

**What works in this mode:** RFID tap, bottle counting (IR break-beam), fill level (HC-SR04), LED + buzzer feedback, points, student page, admin page, Telegram alerts.
**What is skipped:** weight verification (bottles count as 0 g) and the residual flap (no servo, no residual scale).

---

## Step 1 — Firebase: accept 0 g deposits (testing only)

The service only gives points if a deposit weighs at least `minDepositGrams` (normally 10 g). With no scale every bottle weighs 0 g, so set it to 0 while testing.

1. [Firebase console](https://console.firebase.google.com) → project **taptapon** → **Databases & Storage → Realtime Database → Data**.
2. Open **config** → click the value of **minDepositGrams** (`10`) → change it to **`0`** → Enter.

> ⚠️ **Change it back to `10` when the scale is installed.** Otherwise anything waved through the beam earns points.

## Step 2 — Wire the parts

Use **one half** of the breadboard (the long power rails are often split in the middle). Make two shared rows:
- **3.3 V row** ← ESP32 **3V3**
- **GND row** ← ESP32 **GND**

| Part | Connections |
|---|---|
| RC522 RFID | SDA→**D5**, SCK→**D18**, MOSI→**D23**, MISO→**D19**, RST→**D15**, 3.3V→3.3 V row, GND→GND row, IRQ→nothing |
| HC-SR04 | VCC→3.3 V row, GND→GND row, TRIG→**D26**, ECHO→**D27** |
| IR break-beam | emitter + receiver VCC→3.3 V row, both GND→GND row, receiver signal→**D32** |
| LED | **D2** → 220 Ω → LED long leg; short leg → GND row |
| Passive buzzer | + → **D33**, − → GND row |
| Servo, scales | **not connected** |

Point the break-beam emitter and receiver straight at each other.

## Step 3 — Fill in `config.h`

Open `firmware/taptapon_bin/config.h` (it's git-ignored, so passwords stay off GitHub) and replace:
- `WIFI_SSID` / `WIFI_PASSWORD` → a **2.4 GHz** network (a phone hotspot is best; campus Wi-Fi with a login page will not work)
- `DEVICE_PASSWORD` → the device account password (same as the simulator login)

Already set for real hardware: `WIFI_CHANNEL 0`, `FAKE_RFID 0`, `PET_CALIBRATION 337`.

## Step 4 — Upload

1. Arduino IDE → **File → Open →** `firmware/taptapon_bin/taptapon_bin.ino` (do **not** paste into a new sketch; you must see both tabs `taptapon_bin.ino` and `config.h`).
2. Libraries needed: **MFRC522**, **ESP32Servo**, **HX711** (Bogdan Necula).
3. Board **ESP32 Dev Module**, port **COM3** → **Upload** (hold **BOOT** if it stalls at "Connecting…").
4. Serial Monitor at **115200** should show:
   ```
   Connecting to <your wifi>..... connected
   Config: depth 45 cm, full at 80%, min deposit 0 g
   PET scale: NOT CONNECTED (weights read 0 g), residual scale: NOT CONNECTED (weights read 0 g)
   Signed in as the device account
   Ready. Tap a card.
   ```

## Step 5 — Start the rest of the system

- **Points service** (needed for points + Telegram): in a terminal, `cd service` → `npm.cmd start` → `TapTapon service running.`
- **Admin page:** https://taptapon.web.app/admin.html → **Canteen Bin** should change from OFFLINE to **OK** within 30 s.
- **Student page:** https://taptapon.web.app on your phone.

## Step 6 — Register your card

1. Tap the white card on the RC522 → Serial Monitor prints `Card tapped: 7A3F09C1` (your number).
2. Admin page → **Register a student card** → paste the UID, a student number, a display name → **Register card**.

## Step 7 — The full test

| Do this | Expect |
|---|---|
| Tap the card, then pass a bottle (or your hand) through the beam within 30 s | Serial: `Bottle counted (1)` → `Deposit … pet x1, 0 g, tag …` → `service: ok`; LED blinks 3× + happy beep; student page **+1 point** |
| Pass a bottle **without** tapping | `service: no_tap`, two low beeps, no points |
| Tap an **unregistered** card, then a bottle | `service: unknown_tag` |
| Hold your hand ~5 cm under the HC-SR04 | Admin page: bin **FULL** (red); Telegram alert to **TapTapon Janitors**; LED stays on |
| Take your hand away | Bin back to **OK**; Telegram "was emptied. Salamat!" |
| Unplug the ESP32, wait 2+ min | Admin page shows the bin **OFFLINE** |

## Step 8 — When the scale and servo arrive

1. Set `minDepositGrams` back to **10** in Firebase.
2. Connect scale #1 (DT→D16, SCK→D17) and #2 (DT→D4, SCK→D25); set `RESIDUAL_CALIBRATION` after calibrating #2 with `firmware/step6_scale`.
3. Connect the servo (D13) with its own 5 V supply (shared GND) and the 470 µF capacitor.
