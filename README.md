# TapTapon — WasteWise Bin

> **I-tap. I-tapon. Ma-reward.**

A smart two-compartment trash bin for QCU. It alerts janitors when it's full and rewards students for verified recycling. See [CLAUDE.md](CLAUDE.md) for the full design and the Firebase data contract.

| Part | Folder | Status |
|---|---|---|
| Database (rules + seed) | `firebase/` | ✅ Phase 1 |
| Simulator | `simulator/` | ✅ Phase 2 |
| Service (points + Telegram) | `service/` | ✅ Phase 3 |
| Student dashboard | `dashboard/index.html` | ✅ Phase 4 |
| Admin dashboard | `dashboard/admin.html` | ✅ Phase 5 |
| Firmware (ESP32) | `firmware/` | ✅ Phase 6 (Wokwi) |

## Phase 1 — Firebase setup (one time)

Everything stays on the free **Spark** plan.

1. **Create the project.** Go to <https://console.firebase.google.com> → *Add project* → name it `taptapon`. Google Analytics is optional.
2. **Create the database.** *Databases & Storage → Realtime Database → Create database*. Pick location **Singapore (asia-southeast1)**, then start in **locked mode**.
3. **Import the seed data.** In the *Data* tab, click the ⋮ at the top-right of the data panel (next to the database URL) → *Import JSON* → *Browse* → choose `firebase/seed-data.json`.
   ⚠️ Importing at the root **replaces everything** in the database. Only do it on a fresh database.
4. **Publish the security rules.** In the *Rules* tab, replace the contents with `firebase/database.rules.json` and click *Publish*.
5. **Turn on sign-in.** *Security → Authentication → Get started → Email/Password → Enable*.
6. **Create two users.** In *Authentication → Users → Add user*:
   - an admin account, e.g. `admin@taptapon.test`
   - the bin/simulator account, e.g. `device@taptapon.test`

   Copy each user's **User UID**.
7. **Give them roles.** Back in *Databases & Storage → Realtime Database → Data*, add a `roles` node at the root with one child per UID:
   ```
   roles
   ├── <admin UID>:  "admin"
   └── <device UID>: "device"
   ```
   (Click **+** on the root, type the key `roles`, then add each UID under it as a string value.)
8. **Connect the web pages.** *Project settings → General → Your apps → Add app → Web (</>)*. Copy the `firebaseConfig` values into `dashboard/js/firebase-config.js`.
   This config is public by design and safe to commit. The security rules are what protect the data.

### Check that Phase 1 worked
- The *Data* tab shows `config`, `bins`, `tags`, `students`, `rewards` and `roles`.
- In the *Rules* tab, open **Rules Playground**. Simulate a **write** to `/students/2024-00123/points` with value `999` while authenticated as your admin UID. It should be **denied**, because only the service may change points.
- Same playground: simulate an **unauthenticated read** of `/tags`. It should be **denied**.

## Phase 2 — Simulator (virtual bin)

A web page that writes to Firebase exactly like the ESP32 will. **Run it on your own laptop only, never host it.**

### Run it
1. Open the `taptapon` folder in VS Code.
2. Right-click `simulator/index.html` → **Open with Live Server**.
   (No Live Server? In a terminal in the `taptapon` folder, run `py -m http.server 5599`, then open <http://localhost:5599/simulator/>.)
   Double-clicking the file does **not** work: browsers block ES modules on `file://`.
3. Sign in with the **device** account from step 6.

### Test it
| Do this | You should see |
|---|---|
| Sign in | Log: `Signed in as device@… (device)` in green. A red message about `roles` means step 7 is wrong. |
| Drag the fill slider to 85% and release | Log: `bins/bin-01 ← fill 85%…`. In the Firebase *Data* tab, `bins/bin-01/fillPercent` = 85. |
| Click **Empty bin** | `fillPercent` goes back to 0. |
| Pick tag `A1B2C3D4 → 2024-00123`, click **Drop bottle** | A new entry under `deposits` with `type: "pet"`, `count: 1`, `weightGrams: 18`. |
| Pick **No tap**, drop a bottle | The deposit has no `tagUid` field. |
| Click **Drop residual** | The deposit has `type: "residual"`, `count: 0`. |

Points don't change yet. That's the service's job (Phase 3). Once the service is running, each deposit also gets a gold `service → …` line in the log.

## Phase 3 — Service (points + Telegram alerts)

A Node script that runs on a laptop. It's the **only** thing that changes points. Needs Node.js 20.6 or later (`node --version`).

### Set it up (one time)
1. **Get the admin key.** In the Firebase console, click ⚙️ (next to *Project Overview*) → **Project settings** → **Service accounts** tab → **Generate new private key** → **Generate key**.
   A `.json` file downloads. Rename it to `serviceAccountKey.json` and move it into the `service/` folder.
   ⚠️ This key can do *anything* to your database. Never commit it, share it, or paste it into chat. `.gitignore` already blocks it.
2. **Create `.env`.** In `service/`, copy `.env.example` to `.env`. The database URL is already filled in.
3. **Install.** In a terminal inside `service/`, run `npm install`.

### Run and test points
1. In `service/`, run `npm start`. It should print `TapTapon service running.`
2. Any deposits made before the service started are processed right away, and each one prints a line such as `deposit -P3p… : ok, +1 pt → 2024-00123`.
3. Open the simulator, pick tag `A1B2C3D4`, and drop a bottle. The service prints `ok, +1 pt`, and the simulator log shows a gold `service → ok` line.

| Test in the simulator | Expected `reason` |
|---|---|
| Tag `A1B2C3D4`, Drop bottle, 18 g | `ok`, +1 pt |
| **No tap**, Drop bottle | `no_tap` |
| Tag `FFFFFFFF (unregistered card)`, Drop bottle | `unknown_tag` |
| Tag `A1B2C3D4`, **Drop residual** | `residual` |
| Tag `A1B2C3D4`, weight **1** g, Drop bottle | `not_verified` |
| Tag `A1B2C3D4`, drop 21 bottles in one day | the 21st gets `daily_cap`, +0 |
| Slider to 85%, release | Console: `[Telegram not set up] 🗑️ Canteen Bin … 85% full` (once only) |
| Slider to 70% | nothing happens (between 50% and 80%) |
| **Empty bin** | Console: `emptied, alert re-armed`; Telegram (once set up): `✅ … was emptied. Salamat!` |

Unit tests for the rules (no Firebase needed): `npm test`.

### Turn on Telegram alerts
1. In Telegram, message **@BotFather** → `/newbot` → give it a name, e.g. `TapTapon Alerts`, and a username ending in `bot`. BotFather replies with a **token**. Put it in `.env` as `TELEGRAM_BOT_TOKEN=`.
2. Create a Telegram group for the janitors and add your bot to it. Send any message in the group, such as `hi`.
3. In a browser, open `https://api.telegram.org/bot<TOKEN>/getUpdates`, replacing `<TOKEN>` with your token. Find `"chat":{"id":-100…`. That number, including the minus sign, is your `TELEGRAM_CHAT_ID`.
4. Restart the service (Ctrl+C, then `npm start`), push the slider to 85%, and the group gets the alert.

## Phase 4 — Student dashboard

The page students open on their phone next to a bin. No login: they type their student number.

### Run it
Same as the simulator: Live Server on `dashboard/index.html`, or `py -m http.server 5599` and open <http://localhost:5599/dashboard/>.
To try it on your phone, both devices must be on the same Wi-Fi. Use your laptop's IP instead of `localhost`, for example `http://192.168.1.5:5599/dashboard/`. With the `py` server, add `--bind 0.0.0.0`.

### Test it (keep the service running)
| Do this | You should see |
|---|---|
| Enter `2024-00123` | Juan D.'s points, bottles, today's points out of 20, deposits, and his row highlighted in the Top 10 |
| Enter `2024-99999` | "Student number not found…" |
| Enter `abc` | "Use the format 2024-00123." |
| Reload the page | It remembers the last student number |
| In the simulator, tap `A1B2C3D4` and drop a bottle | Points go up by 1 **without reloading**, and a new "+1" deposit appears |
| Earn 10+ points, then tap **10 pts** on the raffle | A confirmation box, then a "Waiting: show your school ID" entry under *My redemptions*. Spendable points drop by 10 until an admin approves or rejects it. |

## Phase 5 — Admin dashboard

For facility staff, on a laptop. Open <http://localhost:5599/dashboard/admin.html> (or Live Server on `dashboard/admin.html`) and sign in with the **admin** account from step 6. A non-admin account is signed out with a message.

**What's on it**
- **Stats:** bottles today, bottles in the last 7 days, segregation rate, and active students. **Segregation rate** = drops at the bottle port that were real bottles (IR count ≥ 1 and weight ≥ `minDepositGrams`) ÷ all drops at the bottle port, whether or not someone tapped.
- **Bin cards:** fill bar (green < 50%, gold 50–79%, red ≥ 80%), **FULL** badge, **OFFLINE** if there's been no heartbeat for 2 minutes, last seen, and totals.
- **Pending redemptions:** oldest first, with the student's current points. **Approve** / **Reject**. The service deducts the points after you approve.

### Test it (keep the service running)
| Do this | You should see |
|---|---|
| Simulator: slider to 85% | The bin card turns red and shows **FULL**. The Telegram alert arrives. |
| Close the simulator tab, wait 2+ minutes | The bin shows **OFFLINE** |
| Student page: request a reward | It appears under *Pending redemptions* instantly |
| **Approve** it | It moves to *Recent decisions* as "Approved · −10 pts", and the student's points drop by 10 |
| Approve a request the student can't afford | "Rejected by system: not enough points". Points unchanged. |
| **Reject** one | "Rejected". Points unchanged, and the student's held points are released. |

### Registering student cards
Under **Register a student card**, enter the card's UID (spaces and colons are fine), the student number, and a display name for new students. A new student is created with 0 points in the same save. If the card already belongs to someone else, the form asks before moving it (for a lost or replaced card); points stay with the student. **Remove** unlinks a lost card.

| Do this | You should see |
|---|---|
| UID `D4E5F6A7`, student `2024-00456` (Maria, giving her a second card), Register | "Registered D4E5F6A7 → 2024-00456". The card appears in the list. |
| Simulator: reload, pick `D4E5F6A7`, Drop bottle | Maria gets +1 |
| UID `E5F6A7B8`, new student `2024-01000`, name `Ana P.` | A new student with 0 points, plus the card |
| UID `D4E5F6A7` again, but for `2024-01000` | A "Move it?" confirmation |

## Phase 6 — Firmware (ESP32)

`firmware/taptapon_bin/`: `taptapon_bin.ino` (the sketch), `config.example.h`, `diagram.json` (Wokwi wiring), `libraries.txt`.

### Run it in Wokwi (no hardware needed)
1. Keep the service running (`npm start` in `service/`) and open the admin page.
2. Go to <https://wokwi.com>, sign in, and start a **new ESP32 project**.
3. Replace the project's files with ours:
   - `sketch.ino` ← contents of `taptapon_bin.ino`
   - `diagram.json` ← contents of `diagram.json`
   - Add a file `libraries.txt` ← contents of `libraries.txt`
   - Add a file `config.h` ← contents of `config.example.h`, then put the **device** password in `DEVICE_PASSWORD`
4. Press ▶. The Serial Monitor should show `connected`, `Config: depth 45 cm…`, `Signed in as the device account`, then `Ready`.
   ⚠️ Keep the Wokwi project **private**, because `config.h` holds the device password.

### Test it
| Do this in Wokwi | You should see |
|---|---|
| Click the ultrasonic sensor and set distance **9 cm** | Serial: `Fill 80%`. Admin: Canteen bin turns **FULL**. Telegram alert arrives. |
| Set distance back to **45 cm** | Fill 0%, alert re-armed, "emptied" message on Telegram |
| Type `A1B2C3D4` in the Serial Monitor + Enter | `Card tapped: A1B2C3D4` and a beep |
| Click the PET load cell, set **0.02 kg**, then press **IR beam** | `Bottle counted (1)`, then after 1.5 s `Deposit … pet x1, ~20 g`, `service: ok`, 3 LED blinks. Juan's points +1. |
| Press **IR beam** without adding weight | `service: not_verified`, two low beeps |
| Press **Residual flap** (servo swings), raise the residual load cell weight during the 5 s | A `residual` deposit |
| Stop the simulation, wait 2+ min | Admin shows the bin **OFFLINE** |

Wokwi's 5 kg load cell needs `PET_CALIBRATION 0.42` (raw 0–2100 = 0–5 kg). If the weight still reads wrong, the formula is: new value = old value × reported grams ÷ real grams.

### On real hardware (when the parts arrive)
Full build guide (parts list, wiring, calibration, booth setup): [docs/BUILD.md](docs/BUILD.md).

1. Arduino IDE → install the **esp32** board package, plus the libraries **ESP32Servo**, **HX711** (bogde) and **MFRC522**.
2. Copy `config.example.h` to `config.h` next to the sketch. Set your Wi-Fi, `WIFI_CHANNEL 0`, `FAKE_RFID 0`, and the device password.
3. Wire per CLAUDE.md §2. Remember the 1 kΩ/2 kΩ divider on ECHO.
4. Calibrate both load cells with a known weight, and measure the real inside depth into `/config/binDepthCm`.
5. Power on with **both compartments empty** (the scales zero themselves at boot).

## Online (Firebase Hosting)

The student and admin pages are hosted for free on Firebase Hosting:
- Student page: **https://taptapon.web.app** (QR code for the booth: `docs/qr-student-page.png` / `.svg`)
- Admin page: **https://taptapon.web.app/admin.html** (admin login required)

Only the `dashboard/` folder is published (`firebase.json`). The simulator is never hosted.

After changing anything in `dashboard/`, publish it again from the `taptapon` folder (PowerShell needs `npx.cmd`, not `npx`):
```
npx.cmd -y firebase-tools@latest deploy --only hosting
```
The points service still has to run on a laptop (`npm start` in `service/`) for points and Telegram alerts.

## Secrets — never commit these
`.gitignore` already covers them:
- `service/.env` (Telegram token, chat ID)
- `service/serviceAccountKey.json` (Firebase Admin key)
- `firmware/taptapon_bin/config.h` (Wi-Fi + device password)
