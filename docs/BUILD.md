# Building the WasteWise Bin prototype (booth version)

The software is done (see `PROGRESS.md`). This guide covers what to buy, how to build the bin, how to bring the electronics up one part at a time, and how to run the booth.

---

## 1. Shopping list

Prices are **rough estimates** for Philippine hobby shops (Shopee, Lazada, e-Gizmo, Deeco). Check before buying.

| # | Part | Qty | Approx. ₱ | Notes |
|---|---|---|---|---|
| 1 | ESP32 DevKit V1 (30 or 38 pin) | 1 (+1 spare) | 300–450 each | Get a spare: they die from wiring mistakes |
| 2 | HC-SR04 ultrasonic sensor | 1 | 60–100 | |
| 3 | SG90 micro servo | 1 | 80–120 | MG90S (metal gear) if the flap is heavy |
| 4 | RC522 RFID kit (reader + white card + blue keyfob) | 1 | 120–200 | The kit's card/fob become demo cards |
| 5 | IR **break-beam** pair (emitter + receiver), 3 mm or 5 mm | 1 pair | 150–400 | Not the "IR obstacle avoidance" module, which is a different sensor |
| 6 | 5 kg load cell + HX711 module | 2 sets | 120–200 each | Buy as a kit with the HX711 |
| 7 | Load cell mounting plates (or acrylic/plywood + M4/M5 screws + spacers) | 2 sets | 100–250 | The cell must bend: fix one end to the bottom plate, the other to the top plate |
| 8 | LED (green, 5 mm) + 220 Ω resistor | 1 | 10 | |
| 9 | Passive buzzer | 1 | 20–40 | |
| 10 | Push button (12 mm, panel mount) | 1 | 20–50 | "Residual" button |
| 11 | Resistors 1 kΩ and 2 kΩ | 1 each | 5 | ECHO voltage divider |
| 12 | Capacitor 470 µF 16 V | 1 | 10 | Across the servo's 5 V, so servo kicks don't reset the ESP32 |
| 13 | Breadboard (or perfboard) + jumper wires (M-M, M-F) | 1 set | 150–250 | Perfboard + soldering is sturdier for a booth |
| 14 | 5 V 2 A USB power adapter + USB cable (data-capable) | 1 | 150–250 | A 10,000 mAh power bank also works at a booth |
| 15 | Two plastic pails/bins that fit side by side, or one bin + divider | 1 | 200–500 | The two compartments |
| 16 | 3" PVC pipe, ~15 cm (bottle port, inner Ø ~80 mm) | 1 | 80–150 | |
| 17 | Plywood/acrylic for lid + front panel, screws, hot glue, zip ties | — | 200–400 | |

**Estimated total: ₱2,500–4,500**, including one spare ESP32. That's useful for your business slides.

---

## 2. Physical layout

```
             ┌────────────── LID ──────────────┐
             │  [HC-SR04 facing DOWN, centered] │
             │    ○ PVC bottle port (Ø80 mm)    │   ← IR emitter on one side of the pipe,
             │      with IR beam across it      │     receiver on the opposite side
             │  ▭ residual flap (servo hinge)   │
             └─────────────────────────────────┘
   FRONT PANEL:  [ RC522 "TAP YOUR ID HERE" ]  (●LED)  (buzzer)  [RESIDUAL button]

   ┌──── PET compartment ────┐ ┌── Residual compartment ──┐
   │   inner bucket          │ │   inner bucket            │
   │ ═══ top plate ═══       │ │ ═══ top plate ═══         │
   │   [load cell #1]        │ │   [load cell #2]          │
   │ ═══ bottom plate ═══    │ │ ═══ bottom plate ═══      │
   └─────────────────────────┘ └───────────────────────────┘
   ESP32 + breadboard in a small box at the back; power cable out the back.
```

Tips:
- **Ultrasonic:** mount it under the lid **above the PET compartment**, pointing straight down. Measure the distance from the sensor to the empty floor (or bucket bottom) and set `/config/binDepthCm` to that number in the Firebase console.
- **Bottle port:** the IR beam must cross the pipe where *every* bottle passes. Drill the emitter and receiver holes facing each other, about 3 cm below the top.
- **Load cells:** the arrow on the cell shows the load direction. The bucket must rest **only** on the top plate and must not touch the walls, or the weight won't register.
- **RC522:** it reads through 1–2 mm of plastic or acrylic, not through metal. Put a sticker on the front: *"I-Tap ang ID dito"*.

---

## 3. Wiring (matches CLAUDE.md §2)

| Part | Part pin → ESP32 | Power |
|---|---|---|
| HC-SR04 | TRIG → **GPIO 26**; ECHO → **1 kΩ → GPIO 27**, and GPIO 27 → **2 kΩ → GND** | VCC → **5 V** (VIN) |
| SG90 servo | signal (orange) → **GPIO 13** | red → **5 V**, brown → GND; 470 µF across 5 V/GND |
| RC522 | SDA/SS → **5**, SCK → **18**, MOSI → **23**, MISO → **19**, RST → **22** | **3.3 V ONLY** (5 V kills it) |
| IR receiver | OUT → **GPIO 32** | receiver VCC → 3.3 V; emitter → 5 V (longer range) |
| HX711 #1 (PET) | DT → **16**, SCK → **17** | VCC → 3.3 V |
| HX711 #2 (residual) | DT → **4**, SCK → **25** | VCC → 3.3 V |
| LED | GPIO **21** → 220 Ω → LED long leg; short leg → GND | |
| Buzzer | **+** → GPIO **33**, **−** → GND | |
| Residual button | one leg → GPIO **14**, other → GND | (internal pull-up) |

**All grounds are connected together.** If the servo makes the ESP32 restart, power the servo from the adapter's 5 V directly instead of the ESP32's VIN pin, with grounds still shared.

---

## 4. Bring-up: one part at a time

Don't wire everything and hope. Add one part, flash, check the Serial Monitor (115200 baud), then add the next. The main sketch prints everything you need.

**Setup (once):** Arduino IDE → Boards Manager → install **esp32 by Espressif**. Library Manager → **ESP32Servo**, **HX711** (by Bogdan Necula), **MFRC522** (by GithubCommunity). Copy `config.example.h` → `config.h` and set:
- `WIFI_SSID` / `WIFI_PASSWORD`, `WIFI_CHANNEL 0`
- `DEVICE_PASSWORD`
- `FAKE_RFID 0` (once the RC522 is wired; keep `1` until then)

| Step | Wire | Check in the Serial Monitor / dashboards |
|---|---|---|
| 1 | ESP32 only (USB) | `connected`, `Signed in as the device account`. Admin shows bin-01 **OK** (not OFFLINE). |
| 2 | + HC-SR04 | `Fill 0% (45.0 cm)`; hand 5 cm in front → `Fill 89%`, the admin bin goes FULL, Telegram alert |
| 3 | + LED, buzzer | LED turns solid when "full" (hand in front) |
| 4 | + IR beam | Pass a bottle through → `Bottle counted (1)` |
| 5 | + HX711 #1 (calibrate, see below) | `PET scale: +500 g` with a 500 g weight |
| 6 | + RC522, `FAKE_RFID 0` | Tap a card → `Card tapped: 7A3F09C1`. Register it on the admin page. |
| 7 | + servo, button, HX711 #2 | Button opens the flap for 5 s → `residual` deposit |
| 8 | Everything | Tap → drop a bottle → `service: ok`, 3 blinks, +1 on the student page |

### Calibrating a load cell (step 5)
1. In `config.h` set `PET_CALIBRATION 1`. Power on with the compartment **empty**.
2. Put something of known weight on it, e.g. a sealed 500 mL water bottle ≈ 520 g (weigh it on a kitchen scale).
3. Read `PET scale: +NNNN g`. With calibration 1, that number is the raw value.
4. `PET_CALIBRATION` = raw ÷ real grams (e.g. 10400 ÷ 520 = **20.0**). Flash again and check it now shows ≈ 520 g.
5. Repeat for the residual cell with `RESIDUAL_CALIBRATION`.

The bin zeros both scales at power-on, so **always power it on empty**.

---

## 5. Running the booth

### Network: use a phone hotspot
- The ESP32 only does **2.4 GHz** Wi-Fi and **can't log in to captive portals** (the "click to accept" campus Wi-Fi). Use a teammate's **phone hotspot** set to 2.4 GHz for the bin **and** the laptop. Put that hotspot's name and password in `config.h`.
- Bring a second phone with data as a backup hotspot.

### What runs where
| Device | Runs |
|---|---|
| Laptop | `npm start` in `service/` (points + Telegram), `py -m http.server 5599`, admin page on screen |
| Bin (ESP32) | powered by adapter or power bank |
| Phone A (on a stand) | Student page showing a demo student's points updating live |
| Phone B | Telegram group "TapTapon Janitors" to show the FULL alert |
| Backup | Simulator tab on the laptop, if the hardware misbehaves |

### Demo cards
Register 3–5 cards before the event (admin page → *Register a student card*): the RC522 kit's card and keyfob, plus teammates' real school IDs. Write each card's student number on a sticky note so visitors can look themselves up.

### A 60-second visitor demo
1. "Tap your ID", then the beep.
2. "Drop the bottle", then the LED blinks 3 times.
3. Point at the phone: the points number stamps up by +1, and the visitor appears on the leaderboard.
4. Hand over the bin to show it's full (hand under the sensor), and the Telegram alert pops up on Phone B.
5. Admin screen: the bin card turns red with a **FULL** badge.

### Day-before checklist
- [ ] Bin powered on **empty**; scales read ~0 g
- [ ] Hotspot on, and the bin shows **OK** on the admin page
- [ ] `npm start` running with no errors
- [ ] Telegram test: hand under the sensor, check the alert arrives, take the hand away
- [ ] Demo cards registered and tested (`service: ok`)
- [ ] Reset demo data if needed: daily caps reset at midnight Manila time
- [ ] Laptop charger, power bank, spare USB cable, spare ESP32, tape, zip ties, hot glue gun
