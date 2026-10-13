# TapTapon — WasteWise Bin

> **I-tap. I-tapon. Ma-reward.**
> A smart two-compartment trash bin for QCU that alerts janitors when it is full and rewards students with points for *verified* recycling.

This file is the project brief for Claude Code. Read it fully before writing any code, and keep it up to date when decisions change.

---

## 1. What we are building

TapTapon is a student technopreneurship project (QCU, BSIT). The product is the **WasteWise Bin**, an IoT system with five parts:

| Part | What it is | Folder |
|---|---|---|
| Firmware | Arduino (C++) code on an ESP32 inside the bin | `firmware/` |
| Database | Firebase Realtime Database (the shared "contract") | `firebase/` |
| Service | Node.js script: awards points + sends Telegram alerts | `service/` |
| Dashboard | Web app for students and admins (HTML/CSS/JS) | `dashboard/` |
| Simulator | Web page that acts like a real bin, for testing without hardware | `simulator/` |

**We are building software first.** The hardware parts have not arrived yet. The simulator writes to Firebase exactly like the real ESP32 will, so the dashboard and service can be finished and tested now. When the hardware is ready, the ESP32 writes to the same paths and nothing else changes.

### How data flows

```
Real bin (ESP32)  ─┐
                   ├─►  Firebase Realtime Database  ─►  Dashboard (students + admin)
Simulator (web)   ─┘              │
                                  └─►  Service (Node)  ─►  awards points
                                                       ─►  Telegram alert to janitors
```

The bin and simulator **only record what happened** (a deposit, a fill reading). They never add points themselves. The **service** decides whether a deposit earns points. This keeps the rules in one place and stops cheating.

---

## 2. Hardware (for reference — the firmware must match this)

| # | Part | Job |
|---|---|---|
| 1 | HC-SR04 ultrasonic sensor | Measures fill level from under the lid |
| 2 | SG90 servo | Opens the residual-waste flap |
| 3 | RC522 RFID reader | Student taps ID to earn points (optional — bin works without a tap) |
| 4 | Bottle intake port, Ø80 mm | One PET bottle at a time |
| 5 | IR break-beam pair | Counts each bottle that passes the port |
| 6 | 5 kg load cell + HX711 (×2) | Confirms each deposit by weight |
| 7 | Status LED + buzzer | Green = points earned, red = full |
| 8 | ESP32 | Wi-Fi to Firebase |

### ESP32 pin map

| Component | Signal | GPIO |
|---|---|---|
| HC-SR04 | TRIG, ECHO* | 26, 27 |
| RC522 (SPI) | SS, SCK, MOSI, MISO, RST | 5, 18, 23, 19, 22 |
| SG90 servo | PWM | 13 |
| IR receiver | OUT (INPUT_PULLUP) | 32 |
| HX711 #1 (recyclables) | DT, SCK | 16, 17 |
| HX711 #2 (residual) | DT, SCK | 4, 25 |
| Status LED | OUT | 21 |
| Buzzer | OUT | 33 |

\* ECHO goes through a 1 kΩ / 2 kΩ voltage divider (5 V → 3.3 V). Servo and HC-SR04 run on the 5 V rail; all grounds are common.

---

## 3. Firebase data structure (THE CONTRACT)

Every part of the system reads and writes these exact paths and field names. **Do not rename fields or change types without updating this section first.** Timestamps are milliseconds since epoch (`Date.now()` / server timestamp).

```jsonc
{
  "config": {
    "pointsPerBottle": 1,
    "dailyPointCap": 20,          // max points one student can earn per day (Asia/Manila)
    "fullThresholdPercent": 80,   // bin counts as "full" at or above this
    "alertResetPercent": 50,      // alert re-arms only after fill drops below this (stops sensor jitter spamming Telegram)
    "minDepositGrams": 10,        // weight increase needed to verify a deposit
    "binDepthCm": 45              // inside depth used to compute fill %
  },

  "roles": {
    "<firebaseAuthUid>": "admin"   // "admin" | "device" — set by hand in the Firebase console
  },

  "bins": {
    "bin-01": {
      "name": "Canteen Bin",
      "location": "QCU Canteen, Ground Floor",
      "fillPercent": 42,           // 0–100, computed by the bin
      "distanceCm": 26.1,          // raw ultrasonic reading
      "status": "ok",              // "ok" | "full" — written by the service only
      "lastSeen": 1760000000000,   // bin heartbeat; dashboard shows "offline" if older than 2 minutes (derived, never stored)
      "alertSent": false,          // service sets true after sending a Telegram alert; resets when fill drops below alertResetPercent
      "totals": { "bottles": 0, "weightGrams": 0 }
    }
  },

  "tags": {
    "A1B2C3D4": "2024-00123"       // RFID tag UID → student number
  },

  "students": {
    "2024-00123": {
      "name": "Juan D.",           // display name only (first name + last initial) — this node is public (Data Privacy Act)
      "points": 0,
      "totalBottles": 0,
      "daily": { "2026-10-13": 5 },  // points earned per Asia/Manila day; used for the daily cap
      "createdAt": 1760000000000
    }
  },

  "deposits": {
    "<pushId>": {
      "binId": "bin-01",
      "tagUid": "A1B2C3D4",        // null if nobody tapped
      "type": "pet",               // "pet" (recyclables port) | "residual"
      "count": 1,                  // bottles counted by the IR beam
      "weightGrams": 18,           // weight increase measured by the load cell
      "timestamp": 1760000000000,
      // ---- written ONLY by the service ----
      "processed": false,
      "studentId": "2024-00123",
      "pointsAwarded": 1,
      "reason": "ok"               // "ok" | "no_tap" | "unknown_tag" | "residual" | "not_verified" | "daily_cap"
    }
  },

  "rewards": {
    "reward-print": { "name": "Free print (5 pages)", "cost": 30, "sponsor": "QCU Library", "active": true },
    "reward-snack": { "name": "₱10 off at the canteen", "cost": 50, "sponsor": "QCU Canteen", "active": true },
    "reward-raffle": { "name": "Monthly raffle entry", "cost": 10, "sponsor": "TapTapon", "active": true }
  },

  "redemptions": {
    "<pushId>": {
      "studentId": "2024-00123",
      "rewardId": "reward-snack",
      "cost": 50,
      "status": "pending",         // "pending" | "approved" | "rejected" — admin sets approved/rejected
      "timestamp": 1760000000000,
      // ---- written ONLY by the service ----
      "processed": false,          // true once points were deducted (or the redemption was refused)
      "reason": "ok"               // "ok" | "insufficient_points"
    }
  }
}
```

### Rules the service enforces when processing a new deposit

1. **Claim first:** run a transaction that flips `processed` from false/missing to `true`. If it was already `true`, skip. (Claiming before awarding means a crash can lose one deposit's points but can never award them twice.)
2. If `tagUid` is null → `reason: "no_tap"`, 0 points (still counts toward bin totals).
3. If `tagUid` is not in `/tags` → `reason: "unknown_tag"`, 0 points.
4. If `type` is not `"pet"` → `reason: "residual"`, 0 points.
5. If `count < 1` **or** `weightGrams < config.minDepositGrams` → `reason: "not_verified"`, 0 points.
6. If `students/{id}/daily/{today}` ≥ `config.dailyPointCap` (Asia/Manila day) → `reason: "daily_cap"`, 0 points.
7. Otherwise award `count × config.pointsPerBottle` (trimmed so the daily cap is not exceeded), using **one transaction on `students/{id}`** that updates `points`, `daily/{today}` and `totalBottles` together.
8. Always add to `bins/{binId}/totals`, then write `studentId`, `pointsAwarded`, `reason` on the deposit.

### Redemption rule

- A student creates a redemption with `status: "pending"`. Points are **not** touched yet.
- The student claims the reward in person; the admin checks their ID and sets `status` to `"approved"` or `"rejected"`. (Anyone who knows a student number can create a pending request — the in-person ID check is what stops abuse.)
- When the service sees `status: "approved"` and `processed` is not true, it deducts `cost` with a transaction on `students/{id}/points`. If the student no longer has enough points, it sets `status: "rejected"`, `reason: "insufficient_points"`. Then it sets `processed: true`.

### Fill alert rule

- `fillPercent = clamp((binDepthCm − distanceCm) / binDepthCm × 100, 0, 100)`.
- When `fillPercent ≥ fullThresholdPercent` and `alertSent` is false → send one Telegram message, set `status: "full"` and `alertSent: true`.
- When `fillPercent < alertResetPercent` (bin emptied) → set `status: "ok"` and `alertSent: false`. Between the two values nothing changes (hysteresis).
- Alert text: `🗑️ {name} ({location}) is {fillPercent}% full. Please empty it.`

---

## 4. Folder structure

```
taptapon/
├── CLAUDE.md                 ← this file
├── README.md                 ← how to run each part (keep updated)
├── .gitignore                ← must ignore .env, service-account keys, config.h, node_modules
├── firebase/
│   ├── database.rules.json   ← security rules
│   └── seed-data.json        ← sample config, bins, students, tags, rewards (import into Firebase)
├── simulator/
│   ├── index.html            ← "virtual bin" control panel
│   └── simulator.js
├── dashboard/
│   ├── index.html            ← student view: look up student number → points, history, rewards, leaderboard
│   ├── admin.html            ← admin view (login required): bin status, stats, redemptions
│   ├── css/styles.css
│   └── js/
│       ├── firebase-config.js   ← public web config (safe to commit), imported by every page
│       ├── student.js
│       └── admin.js
├── service/
│   ├── package.json
│   ├── index.js              ← listens to Firebase, awards points, sends Telegram alerts
│   ├── .env.example          ← TELEGRAM_BOT_TOKEN, TELEGRAM_CHAT_ID, FIREBASE_DB_URL
│   └── (serviceAccountKey.json — NOT committed)
├── firmware/
│   └── taptapon_bin/
│       ├── taptapon_bin.ino
│       └── config.example.h  ← Wi-Fi + Firebase credentials template (real config.h is gitignored)
└── docs/                     ← business plan, blueprint, logos
```

---

## 5. Tech choices (keep it simple — this is a student project)

- **Dashboard and simulator:** plain HTML, CSS and JavaScript (ES modules). **No build step, no framework.** Load the Firebase JS SDK (modular, v10 or later) from the official `gstatic.com` CDN. Run locally with the VS Code **Live Server** extension.
- **Service:** Node.js 20+ with `firebase-admin` and `dotenv`. Call the Telegram Bot API with built-in `fetch` (`https://api.telegram.org/bot<TOKEN>/sendMessage`); no extra Telegram library needed.
- **Firmware:** Arduino IDE or PlatformIO, ESP32 board package. Libraries: `MFRC522`, `ESP32Servo`, `HX711`, and `FirebaseClient` by Mobizt (the older "Firebase ESP32 Client" is deprecated).
- **Auth:** Firebase Authentication (email/password). An account's role comes from `/roles/{uid}`, not from its email.
  - `admin` accounts → admin dashboard.
  - one `device` account → used by the simulator and the ESP32 to write `bins` and `deposits`. You type its password into the simulator page (never into code). Still, **the simulator is run locally only, never hosted** — anyone with the device login can fake deposits.
  - Students do **not** log in for the prototype; they look up their points by student number (read-only).
- **Cost:** everything must stay on free tiers (Firebase Spark plan). Do **not** use Cloud Functions (they need the paid Blaze plan) — that is why the service is a Node script we run on a laptop.

### Security rules (summary)

- Anyone can **read** `config`, `bins`, `rewards`, `students`, `deposits` and `redemptions` (needed for lookup, history and leaderboard — so nothing private is stored there).
- Only admins and the device can read `tags`. A signed-in user can read only their own `roles/{uid}`.
- Only the device account can **write** `bins/{id}/fillPercent|distanceCm|lastSeen` of an existing bin and **create** `deposits` (never update them).
- Only the service (admin SDK, bypasses rules) writes `points`, `daily`, `totalBottles`, `totals`, `processed`, `status`/`alertSent` on bins. Rules reject any client change to `points`, `totalBottles` and `daily` — even from admins.
- Only admins can write `config`, `bins` (name/location), `students`, `tags`, `rewards`, and move a redemption from `pending` to `approved`/`rejected`.
- Anyone can create a `redemptions` entry with `status: "pending"` whose `cost` matches the active reward.
- Indexes: `students` on `points`; `deposits` on `studentId`, `processed`; `redemptions` on `studentId`, `status`.

---

## 6. Design direction

- Brand: **TapTapon**. Colors: green `#1E8E4E`, dark green `#0F3D2E`, gold `#F5B800` (gold = points/rewards). Font: Inter.
- The student view is used on **phones**, standing next to a bin — big text, fast to load, mobile-first.
- The admin view is used on a **laptop** by facility staff — scannable bin cards with fill bars, clear "FULL" states.
- Language: simple English; Filipino phrases welcome for flavor (e.g., "Salamat sa pag-segregate!").

---

## 7. Build order (do one phase at a time, test, then move on)

1. **Phase 1 — Foundation:** folder structure, `.gitignore`, `README.md`, `firebase/seed-data.json`, `firebase/database.rules.json`, `dashboard/js/firebase-config.js` (with placeholder values).
2. **Phase 2 — Simulator:** a page with a bin picker, a "Tap ID" dropdown (from `/tags`), "Drop bottle" and "Drop residual" buttons, a fill-level slider, and an "Empty bin" button. Shows a live log of what it wrote.
3. **Phase 3 — Service:** points processing (rules in §3) + Telegram fill alerts + console logs. Test with the simulator.
4. **Phase 4 — Student dashboard:** student-number lookup, points, recent deposits, rewards list with "Redeem" (creates a pending redemption), top-10 leaderboard.
5. **Phase 5 — Admin dashboard:** login, live bin cards (fill %, status, last seen, totals), approve/reject redemptions, simple stats (bottles today/this week, segregation rate).
6. **Phase 6 — Firmware:** ESP32 sketch that matches §2 and §3; write a fake-sensor mode so it can be tested in the Wokwi simulator before the parts arrive.

---

## 8. Working rules for Claude Code

- Explain what you are about to build before each phase, then build it. I am an IT student — explain *why* in short comments where the logic is not obvious, so the team can defend it.
- Never commit or print secrets: Telegram bot token, Firebase service-account key, Wi-Fi password. Use `.env` / `config.h` and provide `.example` templates.
- Keep the data contract in §3 as the single source of truth. If code needs a new field, update §3 first.
- Prefer small, readable files over clever abstractions.
- After each phase, update `README.md` with exact steps to run and test it.
- Ask before adding any paid service or a heavy dependency.
