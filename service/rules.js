// The rules from CLAUDE.md §3 as plain functions — no Firebase here, so they can be unit-tested.

// "YYYY-MM-DD" in Asia/Manila. The daily cap resets at Manila midnight, not UTC midnight.
// (The en-CA locale happens to format dates as YYYY-MM-DD.)
export const manilaDay = (ms) =>
  new Intl.DateTimeFormat("en-CA", { timeZone: "Asia/Manila" }).format(ms);

// Rules 2–5: why a deposit earns nothing, or null if it may earn points.
// studentId is null when the tag isn't registered.
export function rejectReason(deposit, studentId, config) {
  if (!deposit.tagUid) return "no_tap";
  if (!studentId) return "unknown_tag";
  if (deposit.type !== "pet") return "residual";
  if (deposit.count < 1 || deposit.weightGrams < config.minDepositGrams) return "not_verified";
  return null;
}

// Rules 6–7: points to award, trimmed so today's total never passes the cap.
export function pointsToAward(count, earnedToday, config) {
  return Math.max(0, Math.min(count * config.pointsPerBottle, config.dailyPointCap - earnedToday));
}

// Fill alert rule with hysteresis: "alert", "reset", or null (do nothing).
// Between alertResetPercent and fullThresholdPercent nothing changes, so sensor jitter can't spam alerts.
export function fillAction(bin, config) {
  if (bin.fillPercent >= config.fullThresholdPercent && !bin.alertSent) return "alert";
  if (bin.fillPercent < config.alertResetPercent && bin.alertSent) return "reset";
  return null;
}
