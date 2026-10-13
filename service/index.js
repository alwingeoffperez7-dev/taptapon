// TapTapon service: the only part allowed to change points (CLAUDE.md §3).
// Uses the Admin SDK, which bypasses the security rules. Run with: npm start
import { initializeApp, applicationDefault } from "firebase-admin/app";
import { getDatabase, ServerValue } from "firebase-admin/database";
import { rejectReason, pointsToAward, fillAction, manilaDay } from "./rules.js";

const { FIREBASE_DB_URL, TELEGRAM_BOT_TOKEN, TELEGRAM_CHAT_ID } = process.env;
if (!FIREBASE_DB_URL) throw new Error("FIREBASE_DB_URL missing — copy .env.example to .env");

// applicationDefault() reads the key file named by GOOGLE_APPLICATION_CREDENTIALS in .env.
initializeApp({ credential: applicationDefault(), databaseURL: FIREBASE_DB_URL });
const db = getDatabase();

let config = (await db.ref("config").get()).val();
db.ref("config").on("value", (snap) => (config = snap.val())); // admin edits apply without a restart

// Claims a record by flipping its "processed" flag. Only one caller can win, so nothing is handled twice.
// (A transaction may first run on an empty cache; Firebase then retries it with the real value.)
async function claim(ref) {
  const result = await ref.child("processed").transaction((done) => (done ? undefined : true));
  return result.committed;
}

// ---- Deposits ----
async function processDeposit(snap) {
  const id = snap.key;
  const d = snap.val();
  // Rule 1: claim before awarding — a crash can lose one deposit's points but never double them.
  if (!(await claim(snap.ref))) return;

  const taggedId = d.tagUid ? (await db.ref(`tags/${d.tagUid}`).get()).val() : null;
  const studentId = taggedId && (await db.ref(`students/${taggedId}`).get()).exists() ? taggedId : null;
  let reason = rejectReason(d, studentId, config);
  let points = 0;

  if (!reason) {
    // Rule 7: points, today's total and bottle count change together in one transaction,
    // so two bottles dropped at the same moment can't both slip past the daily cap.
    const day = manilaDay(d.timestamp);
    await db.ref(`students/${studentId}`).transaction((s) => {
      if (!s) return s;
      const earned = s.daily?.[day] ?? 0;
      points = pointsToAward(d.count, earned, config);
      s.points = (s.points ?? 0) + points;
      s.totalBottles = (s.totalBottles ?? 0) + d.count;
      s.daily = { ...s.daily, [day]: earned + points };
      return s;
    });
    reason = points > 0 ? "ok" : "daily_cap";
  }

  // Rule 8: one multi-path update, so totals and the verdict are saved together.
  await db.ref().update({
    [`bins/${d.binId}/totals/bottles`]: ServerValue.increment(d.count),
    [`bins/${d.binId}/totals/weightGrams`]: ServerValue.increment(d.weightGrams),
    [`deposits/${id}/studentId`]: studentId,
    [`deposits/${id}/pointsAwarded`]: points,
    [`deposits/${id}/reason`]: reason,
  });
  console.log(`deposit ${id}: ${reason}, +${points} pt${studentId ? ` → ${studentId}` : ""}`);
}

// ---- Redemptions ----
async function processRedemption(snap) {
  const id = snap.key;
  const r = snap.val();
  if (r.processed || !(await claim(snap.ref))) return;

  let paid = false;
  await db.ref(`students/${r.studentId}/points`).transaction((points) => {
    if (points === null) return null; // empty cache on first run — Firebase retries with the real value
    paid = points >= r.cost;
    return paid ? points - r.cost : undefined; // undefined = abort, points stay the same
  });

  await snap.ref.update(paid ? { reason: "ok" } : { status: "rejected", reason: "insufficient_points" });
  console.log(`redemption ${id}: ${paid ? `-${r.cost} pt` : "rejected, not enough points"} → ${r.studentId}`);
}

// ---- Fill alerts ----
async function checkBin(snap) {
  const bin = snap.val();
  const action = fillAction(bin, config);
  if (action === "reset") {
    await snap.ref.update({ status: "ok", alertSent: false });
    console.log(`bin ${snap.key}: emptied, alert re-armed`);
    // Lets the other janitors know it's handled. A failed send here just gets logged, nothing to retry.
    await sendTelegram(`✅ ${bin.name} (${bin.location}) was emptied. Salamat!`)
      .catch((err) => console.error(`bin ${snap.key}: emptied message failed — ${err.message}`));
  }
  if (action !== "alert") return;

  // Mark first, so the 30-second heartbeat can't trigger a second alert while Telegram is still sending.
  await snap.ref.update({ status: "full", alertSent: true });
  try {
    await sendTelegram(`🗑️ ${bin.name} (${bin.location}) is ${bin.fillPercent}% full. Please empty it.`);
    console.log(`bin ${snap.key}: ${bin.fillPercent}% full, alert sent`);
  } catch (err) {
    await snap.ref.update({ alertSent: false }); // try again on the next reading
    console.error(`bin ${snap.key}: alert failed — ${err.message}`);
  }
}

async function sendTelegram(text) {
  if (!TELEGRAM_BOT_TOKEN || !TELEGRAM_CHAT_ID) return console.log(`[Telegram not set up] ${text}`);
  const res = await fetch(`https://api.telegram.org/bot${TELEGRAM_BOT_TOKEN}/sendMessage`, {
    method: "POST",
    headers: { "content-type": "application/json" },
    body: JSON.stringify({ chat_id: TELEGRAM_CHAT_ID, text }),
  });
  if (!res.ok) throw new Error(`Telegram ${res.status}: ${await res.text()}`);
}

// ---- Listeners ----
// A failure on one record is logged and skipped; it never stops the service.
const safely = (fn) => (snap) => fn(snap).catch((err) => console.error(`${snap.key}: ${err.message}`));

// Unprocessed deposits have no "processed" field, so equalTo(null) also picks up ones missed while offline.
db.ref("deposits").orderByChild("processed").equalTo(null).on("child_added", safely(processDeposit));
db.ref("redemptions").orderByChild("status").equalTo("approved").on("child_added", safely(processRedemption));
db.ref("bins").on("child_added", safely(checkBin));
db.ref("bins").on("child_changed", safely(checkBin));

console.log("TapTapon service running. Press Ctrl+C to stop.");
