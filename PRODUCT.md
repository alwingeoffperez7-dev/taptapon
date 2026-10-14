# Product

<!-- impeccable:product-schema 1 -->

## Platform

web

## Users
- **Students at QCU**, standing beside a WasteWise Bin with their phone (mobile web, no login). Job: drop a PET bottle, tap their school ID, see the point land, check what they can redeem.
- **Facility staff / admins** at a laptop. Job: see which bins are full or offline, approve or reject reward claims after checking a student's ID, register student RFID cards.
- **Janitors** receive Telegram alerts only; they don't use the web UI.
- **Technopreneurship defense panel**: the system is demoed live at the defense, then piloted for real. Both audiences matter equally (confirmed 2026-10-14).

## Product Purpose
A smart two-compartment bin (PET bottles + residual) for Quezon City University that alerts janitors when it's full and rewards students with points for *verified* recycling. Success: students segregate because points are fast, visible and redeemable; staff empty bins before they overflow.

## Positioning
Points are only awarded for deposits verified by both an IR beam count and a load-cell weight increase, and only by a server-side service, so the rewards can't be gamed by tapping without recycling. Rewards are real campus perks (printing, canteen discount, raffle) from campus sponsors.

## Operating Context
- Student view is used outdoors/in the canteen on a phone, often one-handed, right after dropping a bottle; it must load fast and read at a glance.
- Admin view is used on a laptop by facility staff during the day; scanned, not studied.
- Rewards are claimed in person: the student shows their school ID at the sponsor's counter, then an admin approves.
- Prototype runs on Firebase's free Spark plan; the service runs on a laptop.

## Capabilities and Constraints
- Plain HTML/CSS/JS (ES modules), no build step, no framework; Firebase JS SDK from gstatic CDN.
- Student page: lookup by student number, live points, today's points vs daily cap, rewards with Redeem (pending), my redemptions, recent deposits, top-10 leaderboard.
- Admin page: login, stats, live bin cards (fill %, FULL, OFFLINE), approve/reject redemptions, register/remove RFID cards.
- Simulator page: a developer tool imitating the ESP32; local use only.
- Terms: points (not coins), deposit, bin, redeem/redemption, tap (RFID), segregate.

## Brand Commitments
- Name: **TapTapon**. Product: **WasteWise Bin**. Tagline: **"I-tap. I-tapon. Ma-reward."**
- A logo exists (confirmed by the user) but the file is not in the repo yet. UI must reserve a place for it and use it once added; until then a text wordmark stands in. Do not invent a different logo.
- The user asked for a green-based palette (2026-10-14).
- Voice: Taglish flavor: English UI with Filipino touches ("Salamat sa pag-segregate!", "Ma-reward").

## Evidence on Hand
- Seed/test data only (students Juan D., Maria S., Jose R., Alwin P; bins Canteen and Library). No real usage numbers, testimonials or sponsor agreements yet; do not fabricate them.

## Product Principles
1. Proof over promise: show why a deposit did or didn't earn points.
2. Instant feedback at the bin: the point should appear before the student walks away.
3. Fair by design: caps, holds and verification are visible, not hidden.
4. Staff time is scarce: full/offline states must be unmissable.

## Accessibility & Inclusion
- Mobile-first, readable outdoors (strong contrast, large type for key numbers), WCAG AA text contrast, visible focus, works one-handed.
