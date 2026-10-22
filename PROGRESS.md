# Progress — TapTapon

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

## Next up
1. ~~Push to GitHub~~ ✅ https://github.com/alwingeoffperez7-dev/taptapon (branch `main`). After each session: `git push`.
2. **Build the physical prototype for the booth** (prof approved the proposal, 2026-10-22): follow `docs/BUILD.md` (parts list, layout, wiring, one-part-at-a-time bring-up, load-cell calibration, booth setup). Hardware steps: `config.h` with campus Wi-Fi, `WIFI_CHANNEL 0`, `FAKE_RFID 0`; calibrate both load cells; measure the real depth into `/config/binDepthCm`; tap each real ID and register its UID on the admin page.
3. Optional later: pin Google's root CA instead of `setInsecure()` in the firmware (`ponytail:` comment in `taptapon_bin.ino`); stats query with a `timestamp` index if deposits grow past ~1000 (`ponytail:` comment in `admin.js`).

## Key decisions (beyond CLAUDE.md)
- Firmware talks to Firebase over the REST API with built-in `HTTPClient` (no FirebaseClient library).
- Residual flap is opened by a push button on GPIO 14 (added to CLAUDE.md §2).
- One status LED on GPIO 21: 3 blinks + beep = points, solid = full, two low beeps = no points.
- Service uses Node's `--env-file` (no dotenv). Deposits and redemptions are claimed before acting, so two running services never double-award.

## Notes for Claude
- Local test server: `.claude/launch.json` config `static` (`py -m http.server 5599`). `python` isn't on the PATH for background jobs; use `py`.
- Never ask the user to paste passwords or the service-account key into chat. Secrets (git-ignored): `service/.env`, `service/serviceAccountKey.json`, `firmware/taptapon_bin/config.h`.
- Live DB URL: `https://taptapon-default-rtdb.asia-southeast1.firebasedatabase.app` (public reads work via REST for quick checks).
- Wokwi's 5 kg HX711 reads raw 0–2100 → calibration 0.42 raw/g.
