// Admin view: live bin status, stats, and approving/rejecting redemptions.
// Reads are public; the only write here (redemption status) is allowed for admins by the rules.
import { db, auth } from "./firebase-config.js";
import {
  ref, query, limitToLast, onValue, get, update,
} from "https://www.gstatic.com/firebasejs/10.12.2/firebase-database.js";
import {
  signInWithEmailAndPassword, signOut, onAuthStateChanged,
} from "https://www.gstatic.com/firebasejs/10.12.2/firebase-auth.js";
import { manilaDay, when, el, row, emptyRow, toList } from "./ui.js";

const $ = (id) => document.getElementById(id);
const OFFLINE_AFTER_MS = 2 * 60 * 1000; // CLAUDE.md §3: offline if no heartbeat for 2 minutes
const DAY_MS = 24 * 60 * 60 * 1000;
// ponytail: stats use the newest 1000 deposits, computed in the browser.
// Fine for a pilot; add ".indexOn": "timestamp" and a startAt() range query when volume grows.
const DEPOSIT_WINDOW = 1000;

let config = {};
let bins = {};
let students = {};
let rewards = {};
let redemptions = {};
let deposits = [];
let stopListening = [];

// ---- Sign in ----
$("login").addEventListener("submit", async (e) => {
  e.preventDefault();
  setLoginMessage("Signing in…");
  try {
    await signInWithEmailAndPassword(auth, $("email").value, $("password").value);
  } catch (err) {
    setLoginMessage(err.code === "auth/invalid-credential" ? "Wrong email or password." : err.code, true);
  }
});
$("logout").addEventListener("click", () => signOut(auth));

function setLoginMessage(text, isError = false) {
  $("login-msg").textContent = text;
  $("login-msg").classList.toggle("error", isError);
}

onAuthStateChanged(auth, async (user) => {
  stopListening.forEach((stop) => stop());
  stopListening = [];
  $("app").hidden = true;
  $("session").hidden = true;
  if (!user) {
    $("login").hidden = false;
    return;
  }

  // The page only checks this to show the right screen; the security rules enforce it for real.
  const role = (await get(ref(db, `roles/${user.uid}`))).val();
  if (role !== "admin") {
    setLoginMessage(`${user.email} is not an admin account.`, true);
    await signOut(auth);
    return;
  }

  setLoginMessage("");
  $("login").hidden = true;
  $("app").hidden = false;
  $("session").hidden = false;
  $("who").textContent = user.email;

  stopListening = [
    onValue(ref(db, "config"), (snap) => { config = snap.val() || {}; renderBins(); renderStats(); }),
    onValue(ref(db, "bins"), (snap) => { bins = snap.val() || {}; renderBins(); }),
    onValue(ref(db, "students"), (snap) => { students = snap.val() || {}; renderRedemptions(); }),
    onValue(ref(db, "rewards"), (snap) => { rewards = snap.val() || {}; renderRedemptions(); }),
    onValue(ref(db, "redemptions"), (snap) => { redemptions = snap.val() || {}; renderRedemptions(); }),
    onValue(query(ref(db, "deposits"), limitToLast(DEPOSIT_WINDOW)), (snap) => {
      deposits = toList(snap);
      renderStats();
    }),
  ];
});

// "Last seen" and OFFLINE depend on the clock, not just on data changes.
setInterval(() => { if (!$("app").hidden) renderBins(); }, 30_000);

function ago(ms) {
  if (!ms) return "never";
  const s = Math.round((Date.now() - ms) / 1000);
  if (s < 60) return "just now";
  if (s < 3600) return `${Math.floor(s / 60)} min ago`;
  if (s < 86400) return `${Math.floor(s / 3600)} h ago`;
  return when(ms);
}

// ---- Stats ----
function renderStats() {
  const now = Date.now();
  const today = manilaDay(now);
  const weekAgo = now - 7 * DAY_MS;
  let bottlesToday = 0;
  let bottlesWeek = 0;
  let portDrops = 0;
  let realBottles = 0;
  const active = new Set();

  for (const d of deposits) {
    if (d.timestamp < weekAgo) continue;
    if (d.studentId) active.add(d.studentId);
    if (d.type !== "pet") continue;
    portDrops++;
    // Same check the service uses for "not_verified", but independent of whether someone tapped.
    const verified = d.count >= 1 && d.weightGrams >= (config.minDepositGrams ?? 0);
    if (!verified) continue;
    realBottles++;
    bottlesWeek += d.count;
    if (manilaDay(d.timestamp) === today) bottlesToday += d.count;
  }

  const rate = portDrops ? `${Math.round((realBottles / portDrops) * 100)}%` : "—";
  $("stats").replaceChildren(
    stat("Bottles today", bottlesToday),
    stat("Bottles, last 7 days", bottlesWeek),
    stat("Segregation rate", rate, "Real bottles ÷ all drops in the bottle port, last 7 days"),
    stat("Active students", active.size, "Tapped an ID in the last 7 days"),
  );
}

function stat(label, value, sub) {
  const card = el("div", "card stat");
  card.append(el("div", "label", label), el("div", "value", String(value)));
  if (sub) card.append(el("div", "muted", sub));
  return card;
}

// ---- Bins ----
function renderBins() {
  const cards = Object.entries(bins).map(([id, b]) => {
    const level = b.fillPercent ?? 0;
    const full = b.status === "full";
    const offline = Date.now() - (b.lastSeen ?? 0) > OFFLINE_AFTER_MS;
    const badge = full ? ["FULL", "badge-full"] : offline ? ["OFFLINE", "badge-offline"] : ["OK", "badge-ok"];

    const card = el("article", `card bin${full ? " is-full" : ""}${offline ? " is-offline" : ""}`);
    const head = el("div", "bin-head");
    head.append(el("h3", null, b.name), el("span", `badge ${badge[1]}`, badge[0]));

    const bar = el("div", "bar");
    bar.setAttribute("role", "img");
    bar.setAttribute("aria-label", `${level}% full`);
    const fill = el("span", level >= config.fullThresholdPercent ? "hi" : level >= config.alertResetPercent ? "mid" : "lo");
    fill.style.width = `${level}%`;
    bar.append(fill);

    const kg = ((b.totals?.weightGrams ?? 0) / 1000).toFixed(2);
    card.append(
      head,
      el("p", "muted", `${b.location} · ${id}`),
      el("p", "fill-num", `${level}%`),
      bar,
      el("p", "muted", `Last seen ${ago(b.lastSeen)} · ${b.totals?.bottles ?? 0} bottles · ${kg} kg`),
    );
    return card;
  });
  $("bins").replaceChildren(...cards);
}

// ---- Redemptions ----
const rewardName = (r) => rewards[r.rewardId]?.name ?? r.rewardId;
const studentLabel = (r) => `${students[r.studentId]?.name ?? "Unknown"} (${r.studentId})`;

function renderRedemptions() {
  const list = Object.entries(redemptions).map(([id, r]) => ({ id, ...r }));

  const pending = list.filter((r) => r.status === "pending").sort((a, b) => a.timestamp - b.timestamp); // oldest first
  $("pending-count").textContent = pending.length;
  $("pending").replaceChildren(...pending.map((r) => {
    const points = students[r.studentId]?.points ?? 0;
    const short = points < r.cost;
    const li = row(`${studentLabel(r)}: ${rewardName(r)}`,
      `${when(r.timestamp)} · costs ${r.cost} · has ${points} pts${short ? " (not enough)" : ""}`);
    if (short) li.querySelector(".sub").classList.add("error");

    const approve = el("button", "btn-primary", "Approve");
    const reject = el("button", null, "Reject");
    approve.addEventListener("click", () => decide(r.id, "approved", [approve, reject]));
    reject.addEventListener("click", () => decide(r.id, "rejected", [approve, reject]));
    const actions = el("div", "actions");
    actions.append(approve, reject);
    li.append(actions);
    return li;
  }));
  if (!pending.length) $("pending").replaceChildren(emptyRow("Nothing to approve."));

  const decided = list.filter((r) => r.status !== "pending").sort((a, b) => b.timestamp - a.timestamp).slice(0, 10);
  $("decided").replaceChildren(...decided.map((r) => {
    let result;
    if (r.reason === "insufficient_points") result = "Rejected by system: not enough points";
    else if (r.status === "rejected") result = "Rejected";
    else result = r.reason === "ok" ? `Approved · −${r.cost} pts` : "Approved · deducting points…";
    return row(`${studentLabel(r)}: ${rewardName(r)}`, `${when(r.timestamp)} · ${result}`);
  }));
  if (!decided.length) $("decided").replaceChildren(emptyRow("No decisions yet."));
}

async function decide(id, status, buttons) {
  buttons.forEach((b) => (b.disabled = true));
  try {
    await update(ref(db, `redemptions/${id}`), { status });
  } catch (err) {
    alert(`Couldn't save: ${err.message}`);
    buttons.forEach((b) => (b.disabled = false));
  }
}
