# Progress — TapTapon

- 2026-11-12: Revised `docs/CANVASS.md` for PHP2,000 remaining hardware budget using the owned ESP32 starter kit. Planned allowances total PHP1,850 including shipping and a reused-material enclosure, leaving PHP150. Keep two 5kg load cells plus two HX711 boards; no spare ESP32 or extra card pack. Supplier stock conflicts and break-beam electrical compatibility remain unresolved; prices are not a checkout quote. Existing Excel canvass still reflects the older plan.

Read this first in a new session. Full design lives in `CLAUDE.md`, visual system in `DESIGN.md`, run/test steps in `README.md`.

_Last updated: 2026-10-14_

## Status: all software phases done ✅
| Part | Commit | Verified |
|---|---|---|
| Phase 1 — Firebase foundation | `61af0a3` | Rules checked from outside: public reads OK; tags/roles/points writes, fake deposits and ₱0 redemptions denied. `/roles` works. |
| Phase 2 — Simulator | `28416f6` | User-tested: sign-in, fill, heartbeat, pet/residual/no-tap deposits. |
| Phase 3 — Service + Telegram | `c71b9e9`, `605d259` | Live: ok, no_tap, unknown_tag, residual, not_verified, daily_cap. Alert fires once at ≥80%, re-arms below 50% with an "emptied" message. Bot @taptapon_qcu_bot → group "TapTapon Janitors". |
| Phase 4 — Student page | `dab3165` | Phone width, live data: lookup, remembered ID, live points, held points, leaderboard. |
| Phase 5 — Admin page | `3a57982`, `4250c0a` | User-tested: approve → service deducts (Juan 21→11→1), reject, FULL/OFFLINE, card registration (E5F6A7B8 → 2025-00061 "Alwin P"). |
| UI redesign + logo | `f1e45e8`, `3d7ec3e` | "Barangay segregation signage" look (impeccable seed 9a73b8b1). Logo: original in `docs/`, web mark `dashboard/img/logo-mark.png`. |
| Phase 6 — ESP32 firmware | `f5601ca` | Wokwi vs live Firebase: sign-in, heartbeat, card tap, IR count, load cell (cal 0.42), deposit → verdict back at the bin; not_verified (2 g) and daily_cap verified live. |

## Online
- Hosted 2026-10-22 on Firebase Hosting: https://taptapon.web.app (student), /admin.html (admin). Only `dashboard/` is published; simulator returns 404 (by design).
- Booth QR: `docs/qr-student-page.png` / `.svg` → https://taptapon.web.app/
- Redeploy after dashboard changes: `npx.cmd -y firebase-tools@latest deploy --only hosting` (CLI is logged in on this laptop).

## Next up
1. ~~Push to GitHub~~ ✅ https://github.com/alwingeoffperez7-dev/taptapon (branch `main`). After each session: `git push`.
2. **Build the physical prototype for the booth** (prof approved the proposal, 2026-10-22): follow `docs/BUILD.md` (parts list, layout, wiring, one-part-at-a-time bring-up, load-cell calibration, booth setup). Hardware steps: `config.h` with campus Wi-Fi, `WIFI_CHANNEL 0`, `FAKE_RFID 0`; calibrate both load cells; measure the real depth into `/config/binDepthCm`; tap each real ID and register its UID on the admin page.
3. Optional later: pin Google's root CA instead of `setInsecure()` in the firmware (`ponytail:` comment in `taptapon_bin.ino`); stats query with a `timestamp` index if deposits grow past ~1000 (`ponytail:` comment in `admin.js`).

## Key decisions (beyond CLAUDE.md)
- Firmware talks to Firebase over the REST API with built-in `HTTPClient` (no FirebaseClient library).
- Residual flap is opened by a push button on GPIO 14 (added to CLAUDE.md §2).
- One status LED on GPIO 2: 3 blinks + beep = points, solid = full, two low beeps = no points.
- Service uses Node's `--env-file` (no dotenv). Deposits and redemptions are claimed before acting, so two running services never double-award.

## Notes for Claude
- Local test server: `.claude/launch.json` config `static` (`py -m http.server 5599`). `python` isn't on the PATH for background jobs; use `py`.
- Never ask the user to paste passwords or the service-account key into chat. Secrets (git-ignored): `service/.env`, `service/serviceAccountKey.json`, `firmware/taptapon_bin/config.h`.
- Live DB URL: `https://taptapon-default-rtdb.asia-southeast1.firebasedatabase.app` (public reads work via REST for quick checks).
- Wokwi's 5 kg HX711 reads raw 0–2100 → calibration 0.42 raw/g.
- 2026-10-24: Node updated 24.15.0 → 24.22.1 (fixes Node2Shell, CVE-2026-111497). Keep Node ≥ 24.22.1 (or 22.23.4). `npm audit` in service/: 0 vulnerabilities.

- 2026-11-03: Started local web server on 127.0.0.1:5599; Student, admin and simulator routes verified HTTP 200 after starting outside the sandbox. Points service not started or verified in this run.

- 2026-11-03: Prepared financing activity answers for Parts 1–4: illustrative one-bin campus rental model, PHP 5,000 initial funding, PHP 1,500 monthly collections, PHP 900 operating payments and PHP 600 monthly operating surplus. Pricing/customer demand are proposed assumptions, not confirmed sales; hardware estimates use the October canvass.

- 2026-11-14: Work split into two sessions. **Hardware session** (this one): firmware/, docs/BUILD.md, docs/CANVASS.md, parts xlsx, CLAUDE.md §2. **Web session**: dashboard/, simulator/, firebase/ rules + hosting, service/ (web needs). Data-contract changes (CLAUDE.md §3) must be noted here so the other session sees them.

- 2026-11-18 (hardware): Pin map changed so an I2C OLED fits on the ESP32 default pins SDA 21 / SCL 22: status LED 21 → 2, RC522 RST 22 → 15 (CLAUDE.md §2, taptapon_bin.ino, diagram.json, BUILD.md). Added firmware/part_test (no Wi-Fi) with OLED readout. User owns: ESP32 starter kit (incl. OLED, obstacle module, buttons), HC-SR04, SG90, RC522, 2 load cells + 2 HX711 (HX711s unsoldered). Still to buy: IR break-beam; optional 470 µF cap, 40 F-M jumpers.
- 2026-11-18 (hardware): Step-by-step bring-up with the OLED as the readout. Step 1 OLED (firmware/step1_oled) WORKS on the real ESP32 (CP2102, COM3). Next: step2_led, then RFID (firmware/test_rfid_led), then buzzer/servo/HC-SR04 (firmware/test_buzzer_servo_sonar).
- 2026-11-18 (hardware): Step 2 LED works (GPIO 2). Step 3 RFID: RC522 reads the white card with ESP32+RC522 only (firmware/step3b_rfid_only, direct pin-to-pin wires); failed earlier when sharing breadboard power rails (suspect split rail / loose 3.3V). Next: step 4 buzzer + servo + HC-SR04 (firmware/test_buzzer_servo_sonar), then re-add OLED on one rail half.
- 2026-11-18 (hardware): Step 4a buzzer: (pending user result). Step 4b servo: SG90 hums but never moves, on laptop USB AND on a phone charger, with both the ESP32Servo library and hand-made pulses (firmware/step4b_servo_manual) -> servo judged faulty; buy a replacement SG90 (~PHP85) with the IR break-beam order. Moving on to step 4c HC-SR04.
- 2026-11-18 (hardware): Step 4c HC-SR04 WORKS powered from 3V3 with ECHO wired straight to D27 (no divider; safe because the echo is then 3.3 V). Pending: confirm stable readings at 40-50 cm (bin depth 45 cm) before dropping the divider from CLAUDE.md §2 / BUILD.md. Step 4a buzzer result not yet reported.
- 2026-11-18 (hardware) END OF DAY: Buzzer WORKS (step4a). Working on the real ESP32: OLED, LED (GPIO 2), RC522 + white card, HC-SR04 (on 3V3, no divider), passive buzzer. Faulty: SG90 servo (replace). Not yet: HX711 x2 (need soldering), IR break-beam (to buy). NEXT SESSION: (1) check HC-SR04 range at 40-50 cm on 3V3, then update CLAUDE.md §2 + BUILD.md if no divider needed; (2) put parts together on one breadboard half (beware split power rails) and run firmware/part_test with the OLED; (3) shopping: IR break-beam, SG90, 40 F-M jumpers, solder/iron if needed; (4) after soldering: HX711 + load cells, calibrate.
- 2026-11-21 (hardware): User now has soldering iron, IR break-beam, replacement SG90; 470 uF capacitor still shipping (only needed if servo resets the ESP32). Added firmware/step5_breakbeam (beam on D32, both sides on 3V3) and firmware/step6_scale (one HX711 on D16/D17, finds PET_CALIBRATION by typing a known weight). Next: retest new servo (step4b), break-beam, solder HX711 x2 + load cell wires, then step6.
- 2026-11-21 (hardware): IR break-beam WORKS (step5, D32, both sides on 3V3; counts hand passes). Pending: clear PET bottle test (5 drops). New SG90 also only hums: library (ESP32Servo 3.2.1) and hand-made pulses both hum, D13/D25 and VIN/3V3 all tried -> power path through the ESP32 suspected; next: swap power jumpers, then 4xAA battery test with shared GND. Servo test parked for later.
- 2026-11-21 (hardware): HX711 #1 + load cell WORKING on real hardware. Fixes along the way: solder bridge A+/B- and unsoldered A- joint; ESP32-side wiring (DT D16/RX2, SCK D17/TX2 — beware RX0/TX0); bar must be cantilevered (table edge between wired end and the white strain-gauge blob, weight hung at the far tip). Measured: ~230 g item -> +77,500 raw  =>  PET_CALIBRATION ~= 337 (approx; item weight not exact). Put 337 in firmware/taptapon_bin/config.h. Firmware + step6 now use median-of-7 reads and median zeroing (spike-proof); step6 refuses to calibrate on an empty scale. Next: HX711 #2 (RESIDUAL_CALIBRATION) same way, then servo power test.
- 2026-11-21 (hardware): Bin firmware now runs without scales (missing HX711 reads 0 g instead of freezing). config.h created for real hardware (FAKE_RFID 0, WIFI_CHANNEL 0, PET_CALIBRATION 337; Wi-Fi + device password left for the user). Guide docs/TEST_WITHOUT_SCALE.md. For scale-less testing /config/minDepositGrams must be set to 0 by the user in the Firebase console (Claude was not permitted to change live config) — REMEMBER to set it back to 10 when scales are installed.
- 2026-11-21 (hardware) DECISION: Prototype v1 runs WITHOUT load cells (fitted, not connected) and WITHOUT the servo flap. Fill level = HC-SR04; bottles = IR break-beam; owner = RFID. Firebase config/minDepositGrams stays 0 (user sets it in the console). Firmware needs no change (missing HX711 reads 0 g). Scale #1 bench-calibrated (~337) for v2. Video presentation (async) this week: show load cell as bench clip, servo as Wokwi clip or manual flap.
- 2026-11-21 (hardware) END OF DAY: Firmware now counts bottles via a beam INTERRUPT with timing (5-500 ms = bottle, longer = ignored hand) — NOT yet compiled/tested on hardware; step5_breakbeam shows per-pass ms for tuning. Enclosure plan: shoebox 34.3x19.7x12.1 cm stood UPRIGHT (lid = front panel; top = short end with 8.5 cm bottle hole + 5 cm PET-bottle ring with IR beam; ultrasonic behind hole facing down; residual slot 6x3; full-height divider PET 11.5 / residual 8.2 cm). ESP32+breadboard taped outside back wall, 2 wire holes, bundle 1 (top: sonar+beam), bundle 2 (lid: RFID, OLED, LED, buzzer) with ~10 cm slack. Set Firebase config/binDepthCm to measured sensor-to-floor (~32). NEXT SESSION: test step5 timing (bottle vs hand), build the box, wire it, upload taptapon_bin, full test. OPEN QUESTION: add OLED messages to bin firmware (user not yet answered). Video presentation (async) pending; uncommitted work in firmware/ and docs/.
