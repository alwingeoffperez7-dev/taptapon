// Run with: npm test
import { test } from "node:test";
import assert from "node:assert/strict";
import { manilaDay, rejectReason, pointsToAward, fillAction } from "./rules.js";

const config = {
  pointsPerBottle: 1, dailyPointCap: 20, minDepositGrams: 10,
  fullThresholdPercent: 80, alertResetPercent: 50,
};
const bottle = { tagUid: "A1B2C3D4", type: "pet", count: 1, weightGrams: 18 };

test("deposit reasons follow the rule order", () => {
  assert.equal(rejectReason(bottle, "2024-00123", config), null);
  assert.equal(rejectReason({ ...bottle, tagUid: undefined }, null, config), "no_tap");
  assert.equal(rejectReason(bottle, null, config), "unknown_tag");
  assert.equal(rejectReason({ ...bottle, type: "residual", count: 0 }, "2024-00123", config), "residual");
  assert.equal(rejectReason({ ...bottle, weightGrams: 1 }, "2024-00123", config), "not_verified");
  assert.equal(rejectReason({ ...bottle, count: 0 }, "2024-00123", config), "not_verified");
});

test("points are trimmed at the daily cap", () => {
  assert.equal(pointsToAward(1, 0, config), 1);
  assert.equal(pointsToAward(3, 19, config), 1);  // only 1 left before the cap
  assert.equal(pointsToAward(1, 20, config), 0);  // cap reached → daily_cap
});

test("Manila day rolls over at Manila midnight", () => {
  assert.equal(manilaDay(Date.UTC(2026, 9, 13, 15, 59)), "2026-10-13"); // 11:59 PM Manila
  assert.equal(manilaDay(Date.UTC(2026, 9, 13, 16, 0)), "2026-10-14");  // 12:00 AM Manila
});

test("fill alert fires once and re-arms only after emptying", () => {
  assert.equal(fillAction({ fillPercent: 85, alertSent: false }, config), "alert");
  assert.equal(fillAction({ fillPercent: 85, alertSent: true }, config), null);  // already alerted
  assert.equal(fillAction({ fillPercent: 70, alertSent: true }, config), null);  // jitter zone
  assert.equal(fillAction({ fillPercent: 10, alertSent: true }, config), "reset");
  assert.equal(fillAction({ fillPercent: 10, alertSent: false }, config), null);
});
