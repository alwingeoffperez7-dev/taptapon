# Progress — TapTapon

Read this first in a new session. Full design lives in `CLAUDE.md`; run/test steps in `README.md`.

_Last updated: 2026-10-14_

## Done
- **Phase 1 — Foundation** ✅ committed `61af0a3`
  - Firebase project `taptapon` (RTDB asia-southeast1) set up: seed imported, rules published, web config filled.
  - Auth users `admin@taptapon.test` + `device@taptapon.test`; `/roles` node verified working.
  - Rules verified from outside: public reads OK; tags/roles/points writes/fake deposits/₱0 redeem all denied.
- **Phase 2 — Simulator** ✅ committed `28416f6`
  - Tested by user: sign-in, fill slider, heartbeat, pet/residual/no-tap deposits all land in Firebase.
- **Phase 3 — Service** ✅ committed (see git log)
  - Runs live with key + `.env` (both git-ignored). Verified live: ok, no_tap, unknown_tag, residual, not_verified;
    alert fires once at 85%, quiet at 90/70, re-arms at 10%. daily_cap covered by unit tests only.

## Next up (in order)
1. ~~Telegram~~ ✅ bot @taptapon_qcu_bot → group "TapTapon Janitors" (chat ID in `.env`). Live alert sent 2026-10-14.
2. Phase 4 — Student dashboard (`dashboard/index.html`, `student.js`, `css/styles.css`).
3. Phase 5 — Admin dashboard. Phase 6 — Firmware.

## Notes for Claude
- Local test server: `.claude/launch.json` config `static` (`py -m http.server 5599`). `python` isn't on the PATH for background jobs; use `py`.
- Never ask the user to paste passwords or the service-account key into chat.
- Live DB URL: `https://taptapon-default-rtdb.asia-southeast1.firebasedatabase.app` (public reads work via REST for quick checks).
