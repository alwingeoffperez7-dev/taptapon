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
let shownPoints = null; // null = first render for this student, so no animation

// Points tied up in pending requests can't be spent twice.
function availablePoints() {
  const held = Object.values(redemptions)
    .filter((r) => r.status === "pending")
    .reduce((sum, r) => sum + r.cost, 0);
  return (student?.points ?? 0) - held;
}

// ---- Always-on data ----
onValue(ref(db, "config"), (snap) => {
  config = snap.val() || {};
  const ppb = config.pointsPerBottle ?? 1;
  $("how-ppb").textContent = `${ppb} point${ppb === 1 ? "" : "s"}`;
  $("how-cap").textContent = config.dailyPointCap ?? 20;
  renderStudent();
});
onValue(ref(db, "rewards"), (snap) => { rewards = snap.val() || {}; renderRewards(); });
// Ranked by bottles, not spendable points: redeeming a reward shouldn't knock you off the board.
onValue(query(ref(db, "students"), orderByChild("totalBottles"), limitToLast(10)), (snap) => {
  leaders = toList(snap).reverse().filter((s) => s.totalBottles > 0);
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

$("change").addEventListener("click", () => {
  $("lookup").hidden = false;
  $("sid").select();
  $("sid").focus();
});

function setMessage(text, isError = false) {
  $("lookup-msg").textContent = text;
  $("lookup-msg").classList.toggle("error", isError);
}

function showStudent(id) {
  stopListening.forEach((stop) => stop());
  shownPoints = null;
  studentId = id;
  student = null;
  redemptions = {};
  setMessage("Loading…");
  try { localStorage.setItem(SAVED_ID_KEY, id); } catch { /* private mode: just don't remember */ }

  stopListening = [
    onValue(ref(db, `students/${id}`), (snap) => {
      student = snap.val();
      document.querySelectorAll(".needs-student").forEach((node) => (node.hidden = !student));
      $("lookup").hidden = !!student;
      $("how").hidden = !!student;
      setMessage(student ? "" : "Student number not found. Ask the TapTapon booth to register your ID.", !student);
      renderStudent();
      renderRewards();
      renderLeaderboard();
    }),
    onValue(query(ref(db, "redemptions"), orderByChild("studentId"), equalTo(id)), (snap) => {
      redemptions = snap.val() || {};
      renderSpent();
      renderRedemptions();
      renderRewards();
    }),
    onValue(query(ref(db, "deposits"), orderByChild("studentId"), equalTo(id), limitToLast(5)), (snap) => {
      renderDeposits(toList(snap).reverse());
    }),
  ];
}

// ---- Rendering ----

function renderStudent() {
  if (!student) return;
  const points = student.points ?? 0;
  $("name").textContent = student.name;
  $("sid-label").textContent = studentId;
  $("points").textContent = points;
  $("points-label").textContent = points === 1 ? "PT" : "PTS";
  // The one animated moment: a new point gets stamped on, like a painted label.
  if (shownPoints !== null && points > shownPoints) {
    $("points").classList.remove("stamp");
    void $("points").offsetWidth; // restart the animation
    $("points").classList.add("stamp");
  }
  shownPoints = points;

  const bottles = student.totalBottles ?? 0;
  $("bottles").textContent = `${bottles} bottle${bottles === 1 ? "" : "s"}`;

  // Today's cap as one tick per point, so "how many more today" is countable at a glance.
  const today = student.daily?.[manilaDay(Date.now())] ?? 0;
  const cap = config.dailyPointCap ?? 0;
  $("today").textContent = cap ? `${today} / ${cap}` : today;
  $("ticks").replaceChildren(...Array.from({ length: cap }, (_, i) => el("i", i < today ? "on" : "")));
}

// Explains why "24 bottles" can sit next to "3 PTS": the rest went to rewards.
function renderSpent() {
  const spent = Object.values(redemptions)
    .filter((r) => r.status === "approved")
    .reduce((sum, r) => sum + r.cost, 0);
  $("spent").textContent = `${spent} point${spent === 1 ? "" : "s"} spent on rewards`;
  $("spent").hidden = spent === 0;
}

function renderRewards() {
  const list = Object.entries(rewards)
    .filter(([, r]) => r.active)
    .sort(([, a], [, b]) => a.cost - b.cost);
  const available = availablePoints();

  $("rewards").replaceChildren(...list.map(([id, r]) => {
    const button = el("button", "btn-action", `Redeem · ${r.cost}`);
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
    return row(rewards[r.rewardId]?.name ?? r.rewardId, `${when(r.timestamp)} · ${status}`, `−${r.cost}`, r.status === "approved" ? "" : "none");
  }));
  if (!list.length) $("redemptions").replaceChildren(emptyRow("No redemptions yet."));
}

function renderDeposits(list) {
  $("deposits").replaceChildren(...list.map((d) => {
    const pet = d.type === "pet";
    const title = pet ? `${d.count} bottle${d.count === 1 ? "" : "s"}` : "Residual waste";
    const earned = d.pointsAwarded > 0;
    const li = row(title, `${when(d.timestamp)}${earned ? "" : ` · ${REASON_TEXT[d.reason] ?? d.reason}`}`,
      earned ? `+${d.pointsAwarded}` : "+0", earned ? "earned" : "none");
    li.prepend(el("span", `tag ${pet ? "pet" : "residual"}`, pet ? "PET" : "RESIDUAL"));
    return li;
  }));
  if (!list.length) $("deposits").replaceChildren(emptyRow("Tap your ID at a WasteWise Bin to start earning!"));
}

function renderLeaderboard() {
  $("leaderboard").replaceChildren(...leaders.map((s, i) => {
    const li = row(s.name, s.id === studentId ? "You" : undefined, s.totalBottles);
    li.prepend(el("span", "rank", i + 1));
    if (i < 3) li.classList.add("top");
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
