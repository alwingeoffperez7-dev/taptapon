// Virtual bin: writes the same paths and fields as the ESP32 (CLAUDE.md §3).
// It only records events — the service decides points.
import { db, auth } from "../dashboard/js/firebase-config.js";
import { ref, get, onValue, push, update, serverTimestamp } from "https://www.gstatic.com/firebasejs/10.12.2/firebase-database.js";
import { signInWithEmailAndPassword, signOut, onAuthStateChanged } from "https://www.gstatic.com/firebasejs/10.12.2/firebase-auth.js";

const $ = (id) => document.getElementById(id);
const UNKNOWN_TAG = "FFFFFFFF"; // not in /tags — lets us test the "unknown_tag" rule

let bins = {};
let binDepthCm = 45;
let heartbeat = null;

function log(msg, kind = "") {
  const li = document.createElement("li");
  li.className = kind;
  li.textContent = `${new Date().toLocaleTimeString()}  ${msg}`;
  $("log").prepend(li);
}

// ---- Sign in ----
$("login-form").addEventListener("submit", async (e) => {
  e.preventDefault();
  try {
    await signInWithEmailAndPassword(auth, $("email").value, $("password").value);
  } catch (err) {
    log(`Sign-in failed: ${err.code}`, "error");
  }
});
$("logout").addEventListener("click", () => signOut(auth));

onAuthStateChanged(auth, async (user) => {
  $("login-card").hidden = !!user;
  $("bin-card").hidden = $("deposit-card").hidden = !user;
  clearInterval(heartbeat);
  if (!user) return;

  try {
    // Confirms README step 7: without role "device", the rules deny every write.
    const role = (await get(ref(db, `roles/${user.uid}`))).val();
    if (role !== "device") {
      log(`Signed in as ${user.email}, but roles/${user.uid} is ${JSON.stringify(role)} — expected "device". Fix it in the Data tab.`, "error");
      return;
    }
    log(`Signed in as ${user.email} (device)`, "ok");
    await loadDropdowns();
    heartbeat = setInterval(() => sendReading(true), 30_000); // real bin reports every 30 s
  } catch (err) {
    log(`Setup failed: ${err.message}`, "error");
  }
});

async function loadDropdowns() {
  const [config, binsSnap, tags] = await Promise.all(["config", "bins", "tags"].map((p) => get(ref(db, p))));
  binDepthCm = config.val().binDepthCm;
  bins = binsSnap.val() || {};

  $("bin").replaceChildren(...Object.entries(bins).map(([id, b]) => new Option(`${b.name} (${id})`, id)));
  $("tag").replaceChildren(
    new Option("— No tap —", ""),
    ...Object.entries(tags.val() || {}).map(([uid, studentId]) => new Option(`${uid} → ${studentId}`, uid)),
    new Option(`${UNKNOWN_TAG} (unregistered card)`, UNKNOWN_TAG),
  );
  showBinFill();
}

// ---- Fill level ----
function showBinFill() {
  $("fill").value = bins[$("bin").value]?.fillPercent ?? 0;
  $("fill-label").textContent = `${$("fill").value}%`;
}
$("bin").addEventListener("change", showBinFill);
$("fill").addEventListener("input", () => ($("fill-label").textContent = `${$("fill").value}%`));
$("fill").addEventListener("change", () => sendReading()); // fires when the slider is released
$("empty").addEventListener("click", () => {
  $("fill").value = 0;
  $("fill-label").textContent = "0%";
  sendReading();
});

async function sendReading(quiet = false) {
  const binId = $("bin").value;
  const fillPercent = Number($("fill").value);
  // The real bin measures distance and computes %. We go the other way so both numbers stay consistent.
  const distanceCm = +(binDepthCm * (1 - fillPercent / 100)).toFixed(1);
  try {
    await update(ref(db, `bins/${binId}`), { fillPercent, distanceCm, lastSeen: serverTimestamp() });
    bins[binId].fillPercent = fillPercent;
    if (!quiet) log(`bins/${binId} ← fill ${fillPercent}%, distance ${distanceCm} cm`);
  } catch (err) {
    log(`Reading denied: ${err.message}`, "error");
  }
}

// ---- Deposits ----
$("drop-pet").addEventListener("click", () => drop("pet"));
$("drop-residual").addEventListener("click", () => drop("residual"));

async function drop(type) {
  const tagUid = $("tag").value || null;
  const deposit = {
    binId: $("bin").value,
    tagUid, // null → Firebase leaves the field out
    type,
    count: type === "pet" ? 1 : 0, // only the bottle port has the IR beam
    weightGrams: Number($("weight").value),
    timestamp: serverTimestamp(),
  };
  try {
    const depositRef = await push(ref(db, "deposits"), deposit);
    log(`deposits/${depositRef.key} ← ${type}, tag ${tagUid ?? "none"}, ${deposit.weightGrams} g`);
    showServiceResult(depositRef);
  } catch (err) {
    log(`Deposit denied: ${err.message}`, "error");
  }
}

// Waits for the service (Phase 3) to judge the deposit, then logs its verdict.
function showServiceResult(depositRef) {
  const stop = onValue(depositRef, (snap) => {
    const d = snap.val();
    if (!d?.reason) return;
    log(`service → ${d.reason}, +${d.pointsAwarded ?? 0} pt${d.studentId ? ` for ${d.studentId}` : ""}`, "service");
    stop();
  });
}
