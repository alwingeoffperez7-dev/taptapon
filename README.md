# TapTapon — WasteWise Bin

> **I-tap. I-tapon. Ma-reward.**

A smart two-compartment trash bin for QCU. It alerts janitors when it's full and rewards students for verified recycling. See [CLAUDE.md](CLAUDE.md) for the full design and the Firebase data contract.

| Part | Folder | Status |
|---|---|---|
| Database (rules + seed) | `firebase/` | ✅ Phase 1 |
| Simulator | `simulator/` | ✅ Phase 2 |
| Service (points + Telegram) | `service/` | ✅ Phase 3 |
| Student dashboard | `dashboard/index.html` | ⏳ Phase 4 |
| Admin dashboard | `dashboard/admin.html` | ⏳ Phase 5 |
| Firmware (ESP32) | `firmware/` | ⏳ Phase 6 |

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

## Secrets — never commit these
`.gitignore` already covers them:
- `service/.env` (Telegram token, chat ID)
- `service/serviceAccountKey.json` (Firebase Admin key)
- `firmware/taptapon_bin/config.h` (Wi-Fi + device password)
