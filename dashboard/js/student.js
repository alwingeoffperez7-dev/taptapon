// Student view: read-only lookup by student number, plus creating pending redemptions.
// Students don't log in, so everything here uses public reads (see database.rules.json).
import { db } from "./firebase-config.js";
import {
  ref, query, orderByChild, equalTo, limitToLast, onValue, push, serverTimestamp,
} from "https://www.gstatic.com/firebasejs/10.12.2/firebase-database.js";
import { manilaDay, when, el, row, emptyRow, toList } from "./ui.js";

const $ = (id) => document.getElementById(id);
const ID_PATTERN = /^\d{4}-\d{5}$/; // also keeps . # $ [ ] (illegal in Firebase paths) out of queries
const SAVED_ID_KEY = "taptapon.studentId";

const REASON_TEXT = {
  daily_cap: "Daily limit reached",
  not_verified: "Not verified, try again",
  residual: "Residual, no points",
};
const STATUS_TEXT = {
  pending: "Waiting: show your school ID to claim",
  approved: "Claimed ✓",
  rejected: "Rejected",
};

let config = {};
let rewards = {};
let student = null;
let studentId = null;
let redemptions = {};
let leaders = [];
let stopListening = [];

// Points tied up in pending requests can't be spent twice.
function availablePoints() {
  const held = Object.values(redemptions)
    .filter((r) => r.status === "pending")
    .reduce((sum, r) => sum + r.cost, 0);
  return (student?.points ?? 0) - held;
}

// ---- Always-on data ----
onValue(ref(db, "config"), (snap) => { config = snap.val() || {}; renderStudent(); });
onValue(ref(db, "rewards"), (snap) => { rewards = snap.val() || {}; renderRewards(); });
onValue(query(ref(db, "students"), orderByChild("points"), limitToLast(10)), (snap) => {
  leaders = toList(snap).reverse().filter((s) => s.points > 0);
  renderLeaderboard();
});

// ---- Lookup ----
$("lookup").addEventListener("submit", (e) => {
  e.preventDefault();
  const id = $("sid").value.trim();
  if (!ID_PATTERN.test(id)) {
    setMessage("Use the format 2024-00123.", true);
    return;
  }
  showStudent(id);
});

function setMessage(text, isError = false) {
  $("lookup-msg").textContent = text;
  $("lookup-msg").classList.toggle("error", isError);
}

function showStudent(id) {
  stopListening.forEach((stop) => stop());
  studentId = id;
  student = null;
  redemptions = {};
  setMessage("Loading…");
  try { localStorage.setItem(SAVED_ID_KEY, id); } catch { /* private mode: just don't remember */ }

  stopListening = [
    onValue(ref(db, `students/${id}`), (snap) => {
      student = snap.val();
      $("me").hidden = !student;
      setMessage(student ? "" : "Student number not found. Ask the TapTapon booth to register your ID.", !student);
      renderStudent();
      renderRewards();
      renderLeaderboard();
    }),
    onValue(query(ref(db, "redemptions"), orderByChild("studentId"), equalTo(id)), (snap) => {
      redemptions = snap.val() || {};
      renderRedemptions();
      renderRewards();
    }),
    onValue(query(ref(db, "deposits"), orderByChild("studentId"), equalTo(id), limitToLast(10)), (snap) => {
      renderDeposits(toList(snap).reverse());
    }),
  ];
}

// ---- Rendering ----
function renderStudent() {
  if (!student) return;
  $("name").textContent = student.name;
  $("points").textContent = student.points ?? 0;
  $("points-label").textContent = student.points === 1 ? "point" : "points";
  const bottles = student.totalBottles ?? 0;
  $("bottles").textContent = `${bottles} bottle${bottles === 1 ? "" : "s"}`;
  const today = student.daily?.[manilaDay(Date.now())] ?? 0;
  $("today").textContent = config.dailyPointCap ? `${today} / ${config.dailyPointCap}` : today;
}

function renderRewards() {
  const list = Object.entries(rewards)
    .filter(([, r]) => r.active)
    .sort(([, a], [, b]) => a.cost - b.cost);
  const available = availablePoints();

  $("rewards").replaceChildren(...list.map(([id, r]) => {
    const button = el("button", "btn-gold", `${r.cost} pts`);
    button.disabled = !student || available < r.cost;
    button.setAttribute("aria-label", `Redeem ${r.name} for ${r.cost} points`);
    button.addEventListener("click", () => redeem(id, r));
    const need = r.cost - available;
    return row(r.name, need > 0 ? `${r.sponsor} · ${need} more points needed` : r.sponsor, button);
  }));
  if (!list.length) $("rewards").replaceChildren(emptyRow("No rewards right now."));
}

async function redeem(rewardId, reward) {
  const ok = confirm(`Redeem "${reward.name}" for ${reward.cost} points?\n\nShow your school ID at ${reward.sponsor} to claim it. Points are deducted once it's approved.`);
  if (!ok) return;
  try {
    await push(ref(db, "redemptions"), {
      studentId, rewardId, cost: reward.cost, status: "pending", timestamp: serverTimestamp(),
    });
  } catch (err) {
    console.error(err);
    alert("Couldn't send your request. Please try again.");
  }
}

function renderRedemptions() {
  const list = Object.values(redemptions).sort((a, b) => b.timestamp - a.timestamp);
  $("redemptions").replaceChildren(...list.map((r) => {
    const status = r.reason === "insufficient_points" ? "Rejected: not enough points" : STATUS_TEXT[r.status];
    return row(rewards[r.rewardId]?.name ?? r.rewardId, `${when(r.timestamp)} · ${status}`, `−${r.cost}`, r.status === "approved" ? "" : "zero");
  }));
  if (!list.length) $("redemptions").replaceChildren(emptyRow("No redemptions yet."));
}

function renderDeposits(list) {
  $("deposits").replaceChildren(...list.map((d) => {
    const title = d.type === "pet" ? `${d.count} bottle${d.count === 1 ? "" : "s"}` : "Residual waste";
    const earned = d.pointsAwarded > 0;
    return row(title, `${when(d.timestamp)}${earned ? "" : ` · ${REASON_TEXT[d.reason] ?? d.reason}`}`,
      earned ? `+${d.pointsAwarded}` : "+0", earned ? "plus" : "zero");
  }));
  if (!list.length) $("deposits").replaceChildren(emptyRow("Tap your ID at a WasteWise Bin to start earning!"));
}

function renderLeaderboard() {
  $("leaderboard").replaceChildren(...leaders.map((s, i) => {
    const li = row("", undefined, `${s.points} pts`);
    const title = li.querySelector(".title");
    title.append(el("span", "rank", `${i + 1}.`), s.name);
    if (s.id === studentId) li.classList.add("me");
    return li;
  }));
  if (!leaders.length) $("leaderboard").replaceChildren(emptyRow("No points yet. Be the first!"));
}

// Remember this phone's student number between visits.
try {
  const saved = localStorage.getItem(SAVED_ID_KEY);
  if (saved && ID_PATTERN.test(saved)) {
    $("sid").value = saved;
    showStudent(saved);
  }
} catch { /* storage blocked: student just types it again */ }
