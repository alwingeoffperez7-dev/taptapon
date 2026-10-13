# TapTapon — WasteWise Bin

> **I-tap. I-tapon. Ma-reward.**

A smart two-compartment trash bin for QCU. It alerts janitors when it's full and rewards students for verified recycling. See [CLAUDE.md](CLAUDE.md) for the full design and the Firebase data contract.

| Part | Folder | Status |
|---|---|---|
| Database (rules + seed) | `firebase/` | ✅ Phase 1 |
| Simulator | `simulator/` | ⏳ Phase 2 |
| Service (points + Telegram) | `service/` | ⏳ Phase 3 |
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

## Secrets — never commit these
`.gitignore` already covers them:
- `service/.env` (Telegram token, chat ID)
- `service/serviceAccountKey.json` (Firebase Admin key)
- `firmware/taptapon_bin/config.h` (Wi-Fi + device password)
