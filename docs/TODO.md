# TODO — dollfin

Ordered task list. Check items off as they land. Keep one "Now" section at the top.
New tasks go in the phase they belong to.

## Now

- [x] Reminder spike — done, `spikes/reminders/` (D-019)
- [x] P2 — data layer (schema, models, schedule engine, tests)
- [ ] P3 — core UI (medications/regimens, Today, history, cover)
- [ ] Fold the confirmed reminder design into P4 (see below)
- [ ] P10 — Labs (blood results, v1.2; D-024/D-025/D-026) — implemented, needs a device test

## P0 — Research & decisions

- [x] Platform research (`docs/PLATFORM.md`)
- [x] Plan, todo and decision log
- [x] aarch64 only, Jolla Phone 2026 + emulator (D-001)
- [x] Storeman only for v1 (D-007)
- [x] GPL-3.0-or-later (D-008)
- [x] Languages `en`, `nl`, `nb` (D-014)
- [x] Application-site tracking and all-analyte graphs deferred (D-015)
- [x] Qt 5.6/Silica on Storeman, Keel-grade code (D-002)
- [x] Local-calendar reminders + `.ics` fallback (D-006)
- [x] Free-text custom medications, no built-in catalog (D-017)

## P1 — Skeleton

- [x] `git init` + initial commit
- [x] Scaffold from the official `sfdk init` template
- [x] Rename to `harbour-dollfin` (project, desktop, icon, RPM spec)
- [x] Desktop file: `Name=Dollfin`, `OrganizationName=org.dollfin`, `ApplicationName=Dollfin`,
      no permissions declared yet
- [x] `.gitignore` for build artefacts (`RPMS/`, `*.o`, `Makefile`, …)
- [x] `lupdate`/`lrelease` wiring + `en`/`nl`/`nb` translation scaffolds (D-014)
- [x] `LICENSE` (GPL-3.0-or-later)
- [x] Build the RPM in the Platform SDK container (`scripts/build-rpm.sh`, 0 rpmlint errors)
- [x] Reconcile the scaffold with the official `sfdk init` template (files copied from the SDK source)
- [x] Deploy to the Jolla Phone 2026; app launches from the app grid (`scripts/deploy-device.sh`)
- [x] Cover renders
- [x] Note the Keel porting guide so new code stays Keel-grade (D-002)
- [ ] (optional) Install the full Sailfish SDK for the emulator / `sfdk deploy` (`docs/BUILD.md`)

## P2 — Data layer

- [x] Create the SQLite schema (medication, regimen, intake, stock, level, schema_version)
- [x] Medication name is **free text** with optional form/strength/unit fields (D-017)
- [x] Implement connection + `schema_version` migration runner
- [x] Medication model (CRUD)
- [x] Regimen model with schedule expansion (times + interval)
- [x] Intake model (log / edit / delete), generic `site` column for later
- [x] Schedule engine: given a date range, produce scheduled doses
- [x] Unit tests for schema, migration and schedule expansion (`scripts/run-tests.sh`)
- [x] Unit tests for edge cases: DST, month/year boundaries, paused regimens

## P3 — Core UI

- [x] Medications list page + empty state
- [x] Add/edit medication page: dropdown of common HRT meds plus Custom free text (D-017/D-022)
- [x] Add/edit regimen page (dose, route, times, interval, weekly days, dates)
- [x] Today page: today's doses with taken/skipped actions
- [x] History page: list of logged intakes with delete
- [x] Cover: today's doses with a status symbol (tapping the cover opens the app; no cover action)
- [ ] Next-dose countdown on Today
- [ ] Calendar/month view of intakes
- [ ] Fuller error states (DB open failure and save failures surfaced in the UI)
- [x] All user-facing strings wrapped in `qsTr()`

## P4 — Reminders

Spike done (`spikes/reminders/FINDINGS.md`, D-019/D-020/D-021). Mechanism: the app generates
**iCalendar** and hands it to the Calendar UI via `com.jolla.calendar.ui.importIcsData`;
`.ics` file export is the fallback. Reminders are opt-in (D-020).

- [x] iCalendar builder: VEVENT + VALARM, `RRULE` for daily / every N days / weekly, stable
      `UID` per regimen, 1-minute duration, alarm at event start (D-021)
- [x] Consent screen + persisted setting, warning that the chosen calendar may sync (D-020)
- [x] Settings page: reminders on/off, "update calendar reminders", `.ics` export
- [x] Reminder sync calling `com.jolla.calendar.ui.importIcsData` (via `QDBusInterface`)
- [x] `.ics` file export to `~/Downloads/dollfin-reminders.ics`
- [x] Unit tests for the iCalendar builder (`tests/tst_core`)
- [ ] Manual test: alarm fires with the app closed, and after a reboot
- [ ] Document that turning reminders off does not remove events already in the calendar
      (we cannot delete them; the user removes them in the Calendar app)
- [x] Probe removed from `main` (kept on `spike/reminders`); `Calendar` + `Downloads`
      permissions added back

## P5 — Stock

- [x] Stock editor per medication (quantity, unit, low threshold, expiry, **source**)
- [x] Migration to schema v2 (adds `stock.source`)
- [x] Auto-decrement on a taken dose; restore on undo/delete
- [x] Low-stock flag (`quantity <= lowThreshold`) shown in the medications list
- [x] Unit tests for stock + backup (89 checks)
- [ ] Optional low-stock *notification* (currently in-app only)

## P6 — Backup, file access & i18n

- [x] JSON export of all data (generic table dump, future-proof)
- [x] JSON import (replace-all, in a transaction, validated)
- [x] File access via the `Downloads` sailjail permission (D-013)
- [x] Export/import round-trip test
- [x] Translate the UI to Dutch (`nl`) and Norwegian Bokmål (`nb`); usable by adding more

## P7 — Polish

- [ ] Run through the UI Definition of Done checklist
- [ ] Device test matrix (emulator × Jolla Phone 2026 / 5.2): reminders, DST, reboot, low storage
- [ ] Performance: page load, smooth animations, memory (test with `smem`)
- [ ] Accessibility: contrast, tap targets, screen-reader labels
- [ ] Wording review for neutrality and safety
- [x] Remove or gate the development message log — now off unless a `debug` marker file or
      `DOLLFIN_DEBUG` is present, so a release build writes nothing to disk
- [x] App icon (`assets/icon/generate.py` -> `icons/<size>/`, 86/108/128/172)
- [ ] Store screenshots
- [ ] Full UI Definition-of-Done pass on the device

## P8 — Publish (Storeman)

See [`PUBLISH.md`](PUBLISH.md) for the full runbook and the store description text.

- [x] `.spec` finalised, versioning + `%changelog` in place
- [x] Build runbook (`scripts/build-rpm.sh`), tests (`scripts/run-tests.sh`)
- [ ] Install on a clean Jolla Phone 2026 and verify
- [ ] (optional) sign the RPM with a GPG key and publish the key
- [ ] Create the OpenRepos project and upload the RPM
- [ ] Add screenshots and the description
- [ ] Install through Storeman and confirm the listing
- [ ] Tag the release in git

## P9 — Distribution+ (post-v1)

- [ ] Evaluate a Qt 6 / Keel build via Reef (D-002)
- [ ] Submit source to OBS for SailfishOS:Chum
- [ ] Run the Harbour RPM validator; fix what fails
- [ ] Decide whether to submit to the Jolla Store

## P10 — Labs (blood results, v1.2)

Data reuses `level`, no migration (D-024). UIs are **Labs** list + add/edit/delete; graphs are
next (D-026). Ranges are generic Dutch standards and need sign-off (D-025).

- [x] `AnalyteCatalog` in `src/core/analytes.{h,cpp}`: key, label, canonical unit, reference
      text for 35 analytes (HRT and hormones first) + the fixed unit list (D-024/D-025)
- [x] `Store`: add `updateLevel` / `removeLevel` / `level(id)`
- [x] `AppController`: `labs` property, `lab(id)`, `saveLab(map)`, `removeLab(id)`,
      `analyteCatalog()`, `labUnits()`
- [x] `qml/pages/LabsPage.qml`: list newest first, add/edit/delete, empty state
- [x] `qml/pages/LabEditPage.qml`: analyte dropdown (Custom first), locked canonical unit,
      value, draw date, lab, notes, reference shown neutrally
- [x] Labs entry in the Today pull-down
- [x] Unit tests: level CRUD, catalogue/unit integrity, backup round-trip (358 checks)
- [x] New strings translatable; `lupdate`/`lrelease` done for `en`/`nl`/`nb`
- [x] Per-analyte grouping: `Analytes::summarize` + `analyteSummaries` (D-028)
- [x] Labs page = one row per analyte; `AnalyteResultsPage`; Add locks the analyte
- [ ] Device test: pick a test, add under it, edit/delete
- [ ] P7 wording pass on the reference ranges and the “not medical advice” wording
      (D-012/D-025)

## P11 — Lab graphs

One analyte, one unit at a time (D-026/D-028). No chart dependency: QML `Canvas`.

- [x] `AnalyteGraphPage.qml`: Canvas line + points, dated x-axis, unit label
- [x] Handle 0 / 1 point; unit switch when an analyte has more than one
- [x] Reference band only for unambiguous single ranges (D-029)
- [x] “Illustrative only, not medical advice” label (D-012)
- [x] Graph entry in the analyte page’s pull-down
- [ ] Device test: graph with 1 and with several points, band vs no band, unit switch

## Units per dose (D-031)

- [x] Schedule page: "Pills/Patches/Sprays per dose" field, only for countable forms
- [x] Count stored in `regimen.dose`, shown in the dose text, multiplies the logged amount
- [x] Unit test: dose round-trip, `unitsPerDose` default
- [x] Form change on a medication with schedules asks to update them (Now / Later)
- [ ] Device test: pill schedule with 2 and 0.5, gel schedule shows no field,
      Pill -> Gel shows the dialog; Now opens the schedule, Later returns to the list
- [ ] Decide whether stock decrements by units per dose when the stock unit is a unit
      (tablets/patches) rather than doses (D-023)

## Backlog (post-v1)

- [ ] Hormone graphs for **all** tracked analytes (next v1.x, D-026)
- [ ] Application-site tracking (gel / patch / injection)
- [ ] Adherence statistics
- [ ] App lock (PIN via `Sailfish.Secrets`)
- [ ] More languages
- [ ] Import from a mona export
- [ ] Cover quick-actions
- [ ] Optional companion background-service package if calendar reminders prove insufficient
