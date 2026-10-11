# Decision log — dollfin

A running record of decisions and *why* they were made, so future contributors (and future
us) don't relitigate settled questions. **Append-only:** once a decision is `Accepted`, never
edit it; add a new one that supersedes it. (Draft `Proposed`/`Open` entries may be finalised
in place.)

## How to use this log

1. Copy the template below to the end of the list.
2. Give it the next `D-0NN` id and today's date.
3. Set a status:
   - `Open` — raised, not decided
   - `Proposed` — recommendation on the table, awaiting confirmation
   - `Accepted` — current decision
   - `Rejected` — considered and declined
   - `Superseded by D-0NN` — replaced
4. Keep it to a few lines: Context / Decision / Consequences.
5. If a decision touches the schema, code or scope, also update `PLAN.md` / `TODO.md`.

Template:

```md
### D-0NN — Title
- **Date:** YYYY-MM-DD
- **Status:** Proposed
- **Context:** why this came up
- **Decision:** what we chose
- **Consequences:** what follows, including the downside
```

---

## Decisions

### D-001 — Platform target
- **Date:** 2026-10-10 · **Status:** Accepted
- **Context:** Sailfish OS 5.2 is only on the Jolla Phone 2026; 5.1 is broad. Older Xperia
  devices are `armv7hl`.
- **Decision:** Target Sailfish OS 5.x, **aarch64 only**. Test on a **Jolla Phone 2026** and
  in the **emulator**. No `armv7hl` build and no legacy device support.
- **Consequences:** Smaller build and test matrix; unsupported on old Xperia XA2/10.

### D-002 — UI toolkit / Qt version
- **Date:** 2026-10-10 · **Status:** Accepted
- **Context:** Sailfish OS still runs Silica on **Qt 5.6**. **Keel** is a new Qt 6
  reimplementation of Silica, but it is beta, tested only on Jolla Phone 2026 / 5.2.0.18, and
  its apps are distributed through the **Reef** store. Storeman/OpenRepos does not deliver the
  Keel runtime. So "Qt 6 today" effectively means "Reef today".
- **Decision:** Build v1 on **Qt 5.6 + Sailfish Silica** and publish on **Storeman**, which
  gives the best visibility now. Write the code **Keel-grade** (only Silica features Keel
  supports) so a Qt 6 / Keel build can be added later for Reef without a rewrite.
- **Consequences:** We live with Qt 5.6's older feature set for v1. A Qt 6 / Keel / Reef
  release is a post-v1 task (P9), not a rewrite.

### D-003 — Implementation language
- **Date:** 2026-10-10 · **Status:** Accepted
- **Context:** QML-only is quick but hard to test; Python is discouraged for new Harbour apps.
- **Decision:** **C++ backend + QML UI.** Business logic in C++, testable with QtTest.
- **Consequences:** Slightly more setup; unit-testable data and reminder logic.

### D-004 — Storage
- **Date:** 2026-10-10 · **Status:** Accepted
- **Context:** Options: QML `QtQuick.LocalStorage`, QtSql/QSQLITE, or raw `libsqlite3`.
- **Decision:** **SQLite via QtSql (QSQLITE)** in C++, file in the sandboxed app dir
  (`~/.local/share/harbour-dollfin/dollfin.db`).
- **Consequences:** Straightforward migrations and tests; both `libQt5Sql` and `libsqlite3`
  are Harbour-allowed.

### D-005 — Local-first, no network
- **Date:** 2026-10-10 · **Status:** Accepted
- **Context:** The data is highly sensitive.
- **Decision:** The app never connects to a network and never declares the `Internet`
  sailjail permission. No accounts, telemetry or crash reporting.
- **Consequences:** No cloud sync or remote backup. This is a feature, not a limitation.

### D-006 — Reminder delivery mechanism
- **Date:** 2026-10-10 · **Status:** Accepted (reliability verified in P4)
- **Context:** Normal apps are suspended in the background, so an in-app timer cannot fire a
  dose reminder while the app is closed. Options researched in `PLATFORM.md`:
  1. **Calendar integration** — write repeating dose events into a dedicated **local**
     calendar via `nemo-qml-plugin-calendar`; the OS calendar/`timed` delivers the alarms.
  2. **`.ics` export** — the user imports a generated calendar file; no automatic sync.
  3. **Separate background-service package** — a companion RPM using `timed`.
- **Decision:** Primary = **1**, writing to a dedicated **local** calendar. Always offer
  **2** as a fallback and for users who don't want the app touching their calendar. Keep
  **3** as backlog unless both fail.
- **Consequences / hard rule:** Calendar events could sync to a CalDAV/Google account and
  leak HRT medication data. Only write to a **local** calendar; warn the user if that is not
  possible. The P4 spike confirms closed-app delivery, reboot and DST handling.

### D-007 — Distribution channels
- **Date:** 2026-10-10 · **Status:** Accepted
- **Context:** OpenRepos/Storeman, SailfishOS:Chum, Harbour, Reef.
- **Decision:** **OpenRepos / Storeman only for v1.** Chum, Harbour and Reef are future
  options, not v1 work.
- **Consequences:** No Harbour API validator gate, but keep the code inside the allowed APIs
  where practical so Harbour stays possible.

### D-008 — License
- **Date:** 2026-10-10 · **Status:** Accepted
- **Context:** mona is open source; we want the same transparency.
- **Decision:** **GPL-3.0-or-later.**
- **Consequences:** Copyleft. Add a `LICENSE` file before the first public release.

### D-009 — v1 scope
- **Date:** 2026-10-10 · **Status:** Accepted
- **Context:** Scope creep is the main risk to shipping.
- **Decision:** v1 = medications, regimens, Today, reminders (per D-006), history, stock, JSON
  backup, and English/Dutch/Norwegian. Everything else is backlog (see `PLAN.md`).
- **Consequences:** See D-015 for what is explicitly deferred.

### D-010 — Naming and identifiers
- **Date:** 2026-10-10 · **Status:** Accepted
- **Context:** Need a stable binary, package, sandbox identity and author name.
- **Decision:** Display name **Dollfin**; project/repo **dollfin**; binary/package
  **`harbour-dollfin`**; `OrganizationName=org.dollfin`, `ApplicationName=Dollfin`; author
  **lillelilje** (GitHub `lillelilje`).
- **Consequences:** Changing the binary/app name later breaks sailjail data paths and updates.

### D-011 — Backup format
- **Date:** 2026-10-10 · **Status:** Accepted
- **Context:** Users must be able to move or restore their data.
- **Decision:** Plain-text **JSON** export/import of all tables, versioned.
- **Consequences:** Human-readable and future-proof; import must validate and handle version
  mismatches.

### D-012 — No medical advice
- **Date:** 2026-10-10 · **Status:** Accepted
- **Context:** Health apps can cause harm by implying dosing guidance.
- **Decision:** The app records what the user configures. It never recommends doses,
  schedules or medication changes, and presents level simulations as illustrative only.
- **Consequences:** Any feature that computes or visualises levels must carry a clear
  disclaimer and stay opt-in.

### D-013 — Calendar / file access permission
- **Date:** 2026-10-10 · **Status:** Accepted
- **Context:** The chosen reminder path (D-006) uses calendar integration plus `.ics`
  export, and backup (D-011) needs file access.
- **Decision:** Declare the **`Calendar`** permission for local-calendar reminders, plus the
  **narrowest file permission** (`Documents` or `Downloads`) for `.ics` export and JSON
  backup/import. Never `Internet`.
- **Consequences:** Two extra permissions in the desktop file, both outside the
  Harbour-allowed list — fine for Storeman. Exact file permission finalised in P6.

### D-014 — Languages
- **Date:** 2026-10-10 · **Status:** Accepted
- **Context:** Want multi-language support from the start.
- **Decision:** Ship **English (`en`), Dutch (`nl`) and Norwegian Bokmål (`nb`)** in v1. All
  strings go through `qsTr()` and the `lupdate`/`lrelease` workflow from P1 so adding a
  language is cheap. Nynorsk is not planned.
- **Consequences:** Slightly slower UI work; no hardcoded user-facing strings.

### D-015 — Deferred features (backlog, not v1)
- **Date:** 2026-10-10 · **Status:** Accepted
- **Context:** Keep v1 shippable; separate "nice to have" from "must have".
- **Decision:** Defer to backlog:
  - **Application-site tracking** for **gel, patch and injection** (replaces the old
    "injection-site" idea — it should cover every route that has a site).
  - **Hormone graphs for all tracked analytes**, not just estradiol (e.g. testosterone,
    E2, prolactin, as entered), with clear "illustrative, not medical advice" handling.
  - App lock, adherence stats, more languages, mona import, cover quick-actions.
- **Consequences:** Not designed into the v1 schema unless cheap; keep `intake.site` and the
  `level` table generic enough to support them later without a breaking migration.

---

### D-016 — Build environment
- **Date:** 2026-10-10 · **Status:** Accepted
- **Context:** This host is immutable Bazzite with no Docker/VirtualBox and password-gated
  sudo, so installing the full Sailfish SDK (or Platform SDK chroot) is awkward. Meanwhile the
  community Platform SDK **container** image provides a **5.2.0.15 aarch64** target that the
  official SDK lacks.
- **Decision:** Build RPMs with `scripts/build-rpm.sh` using the
  `coderus/sailfishos-platform-sdk` podman image. The full SDK is optional and only needed for
  the emulator or `sfdk deploy`.
- **Consequences:** No emulator; test installs are pushed to the phone by `scp` + `pkcon`.
  We depend on a third-party image (pin the tag) and on rootless podman working
  (needs `ignore_chown_errors = true` for the image's UID 100000). ~6 GB of disk for the image.

---

### D-017 — Custom medications (free text, no built-in catalog)
- **Date:** 2026-10-10 · **Status:** Accepted
- **Context:** HRT regimens vary widely and include compounded or uncommon preparations. The
  reference app (mona) only allows a predefined list, which locks people out. Every user must
  be able to track exactly what they take.
- **Decision:** Medication entry is **free text**. v1 ships **no bundled medication catalog**.
  A medication is a name plus optional form, strength, unit and notes. A small optional
  quick-pick of common names may be added later as a convenience, but custom entry is always
  available and never hidden.
- **Consequences:** One of our differences from mona. Nothing to maintain, translate or go
  stale, and no implied dosing advice. If a quick-pick is added it must not gate or replace
  free-text entry.

---

### D-018 — Reminder mechanism, refined after device recon
- **Date:** 2026-10-10 · **Status:** Accepted
- **Context:** Recon on 5.2.0.18 (`spikes/reminders/FINDINGS.md`) found that `Base.permission`
  already lets every sandboxed app talk to `timed` (`com.nokia.time`), and the `Nemo.Alarms`
  QML plugin can create alarms with no permission. But those alarms are weekday-only
  (no "every N days") and appear in the **Clock app**, leaking medication data to a very
  visible place.
- **Decision:** Keep **D-006** as-is: reminders are OS calendar events in a dedicated **local**
  notebook (full recurrence) with **`.ics` export** as the fallback. Do **not** use
  `Nemo.Alarms` as the default. It may return later as an optional "simple daily alarm" mode.
  The probe in `spikes/reminders/` confirms the calendar path before P4.
- **Consequences:** We still take the `Calendar` permission. If the probe shows the calendar
  write is unreliable, `Nemo.Alarms` (no permission) becomes the fallback before a companion
  background service is considered.

---

### D-019 — Reminders go through the Calendar UI import (supersedes D-006's direct write)
- **Date:** 2026-10-10 · **Status:** Accepted
- **Context:** The probe (`spikes/reminders/FINDINGS.md`) proved a sandboxed third-party app
  cannot see the real calendar DB (`~/.local/share/system/privileged/`, group `privileg`), so
  direct writes via `org.nemomobile.calendar` silently landed in an empty fallback DB. The
  `Calendar` permission instead grants `dbus-user.talk com.jolla.calendar.ui`, and the
  Calendar app exposes `importIcsData(icsString)` / `importFile(fileName)`.
- **Decision:** The app **generates iCalendar itself** and hands it to the Calendar UI via
  `com.jolla.calendar.ui.importIcsData`. It always also offers **`.ics` file export**. No
  direct DB writes and no notebook creation.
- **Consequences:** One-time user confirmation per regimen (and again on schedule change); the
  user picks the notebook in the import UI, so there is no silent sync leak. Recurrence is
  unlimited because we author the `RRULE`. The `Calendar` permission is retained for the
  D-Bus grant; still no background service. D-006's "write to a local notebook directly" is
  void.

---

### D-020 — Calendar reminders are opt-in, with a sync warning
- **Date:** 2026-10-10 · **Status:** Accepted
- **Context:** Handing a dose event to the Calendar means it lands in whatever notebook the
  user picks; if that notebook is a synced CalDAV/Google account, medication names upload to a
  server. This is the one place dollfin's local-only promise can leak.
- **Decision:** Calendar reminders are **opt-in**. Before the first import the app explains,
  in plain language, that the chosen calendar may sync to a cloud, and recommends a local one.
  Nothing is imported without explicit consent. Declining leaves the app fully usable; manual
  `.ics` export stays available.
- **Consequences:** Need a consent screen/flag and honest wording. The user, not the app,
  chooses the notebook, so responsibility is shared and visible.

### D-021 — Reminder event defaults
- **Date:** 2026-10-10 · **Status:** Accepted
- **Context:** Probe feedback: a 15-minute event with a pre-alarm is unnecessary, and a
  before-the-event alarm can be missed if the import takes a while.
- **Decision:** Generate a **1-minute** event (`DURATION:PT1M`) starting at the dose time, with
  the alarm **at the event start** (`TRIGGER:PT0S`). The description carries a plain hint to
  open Dollfin. No clickable link is possible: the Calendar app renders descriptions as plain
  text and exposes no URL field (only `location` is clickable).
- **Consequences:** Once the event is in the calendar, delivery is the OS's job. Logging a dose
  is a separate manual action in Dollfin.

---

### D-022 — Medication, method and unit pickers
- **Date:** 2026-10-10 · **Status:** Accepted
- **Context:** Free-text-only medication entry was clunky, and method/unit were free text too. The
  reference app (mona) forces a fixed list; we want the best of both.
- **Decision:** The medication field is a **dropdown of common HRT medications with "Custom"
  first**; choosing Custom reveals a free-text name field, so any medication is still possible.
  **Method** (pill / gel / patch / injection / …) and **unit** (mg, µg, g, ml, IU, %) are fixed
  dropdowns — no custom units, so entries stay consistent (µ is the micro sign U+00B5).
  **Strength, unit and method live on the medication**; a schedule only carries timing.
- **Consequences:** Refines D-017 (the data model still stores the name as free text). The
  schedule editor no longer asks for dose/unit/route, which were redundant.

---

### D-023 — Stock semantics and backup format
- **Date:** 2026-10-10 · **Status:** Accepted
- **Context:** P5/P6. Stock needs a simple, predictable rule; backup must survive future schema
  changes.
- **Decision:** A stock record keeps **quantity + unit + low threshold + expiry + source**
  (source = prescriber / pharmacy / where it came from, migration v2). Quantity is a **count of
  doses**; each *taken* dose decrements it by one, and undoing/deleting a taken dose adds it
  back. `quantity <= lowThreshold` flags the medication as low. Backups are **JSON: a generic
  dump of every table's columns**, imported replace-all inside a transaction.
- **Consequences:** Decrement is one per logged dose, independent of the dose amount — good
  enough for “how many are left”, revisit if users track partial doses. The generic dump keeps
  working as columns are added; import replaces everything, and the UI warns about that.

---

### D-024 — Labs (blood results) tracking, no schema change
- **Date:** 2026-10-10 · **Status:** Accepted
- **Context:** v1.2 records blood results: hormones first, plus liver, kidney, blood count,
  lipids and the like. The v1 schema already has `level(id, taken_at, analyte, value, unit,
  lab, notes)` and `Store::addLevel()/levels()`, but no UI ever wrote to it, so there is no
  stored data to migrate. Backups already dump `level` generically (D-023).
- **Decision:** Reuse `level` **without a migration**. The feature is called **Labs** in the UI
  and opens from the Today pull-down. `level.analyte` holds a stable catalog **key** for a
  known analyte (`estradiol`) or the user's own text for a **Custom** entry; `taken_at` is the
  **draw date** (local midnight); `unit` is stored per row. Analyte entry is a **dropdown of
  common analytes, HRT first, Custom first** (same pattern as D-017/D-022). **Unit is a fixed
  dropdown, never free text**; a catalog analyte has one canonical unit and the editor locks
  it, while a Custom analyte may pick any listed unit. Display labels are resolved from the key
  at render time, so translations and catalogue edits stay safe.
- **Consequences:** No schema change, no migration, backup round-trips unchanged. `Store`
  gains `updateLevel`/`removeLevel`/`level(id)` next to the existing `addLevel`/`levels`, all
  unit-tested. The catalogue lives in C++ (`src/core/analytes.*`) so it is testable; QML only
  renders it. A result entered in one language still shows the right name after a language
  change.

---

### D-025 — Generic reference ranges (Dutch standards)
- **Date:** 2026-10-10 · **Status:** Accepted (ranges get the P7 wording pass)
- **Context:** The user wants a general best-practice reference range per analyte, not a
  user-entered range. Ranges are lab- and assay-specific, and some (testosterone, haemoglobin)
  are sex-specific; the app must not assume a gender (golden rule 3, D-012). A *target* range
  for HRT hormones would read as dosing advice.
- **Decision:** Ship a **built-in general adult reference range per catalogue analyte**,
  aligned with **Dutch (NVKC/ECLM-style) units and values** (Hb in mmol/L, glucose and lipids
  in mmol/L, HbA1c in mmol/mol). Custom analytes show **no** range. A sex-specific range is
  shown as a neutral `M … · F …` string and never as one assumed range. A context-dependent
  value (estradiol, LH/FSH cycle phase, cortisol time of day) is shown as descriptive text and
  is **not** flagged. The UI states the range factually (`Reference: …`) and adds a neutral
  *below / within / above the general reference* caption in the platform's secondary colour —
  never `Theme.errorColor`, never advice. Ranges are not stored per result; they are looked up
  from the catalogue so a correction applies everywhere.
- **Range table (Dutch general adult values; P7 reads them for tone):**

  | Analyte (key) | Unit | General adult reference |
  |---------------|------|-------------------------|
  | Estradiol (E2) (`estradiol`) | pmol/L | context-dependent (cycle / menopause / HRT) |
  | Estrone (E1) (`estrone`) | pmol/L | context-dependent |
  | Testosterone, total (`testosterone`) | nmol/L | M 8-30 · F 0.5-2.5 |
  | Testosterone, free (`free_testosterone`) | pmol/L | M 170-500 · F < 30 |
  | Prolactin (`prolactin`) | µg/L | M < 15 · F < 25 |
  | LH (`lh`) | U/L | context-dependent (cycle); M 1.5-9 |
  | FSH (`fsh`) | U/L | context-dependent (cycle); M 1.5-12 |
  | SHBG (`shbg`) | nmol/L | M 20-70 · F 40-120 |
  | Progesterone (`progesterone`) | nmol/L | M < 1 · F follicular < 3, luteal 15-80 |
  | DHEA-S (`dhea_s`) | µmol/L | age-dependent; roughly 2-10 |
  | Cortisol (`cortisol`) | nmol/L | morning 150-550 (diurnal) |
  | TSH (`tsh`) | mU/L | 0.4-4.0 |
  | Free T4 (`ft4`) | pmol/L | 8-22 |
  | Free T3 (`ft3`) | pmol/L | 3.5-6.5 |
  | ALT / ALAT (`alt`) | U/L | M < 40 · F < 30 |
  | AST / ASAT (`ast`) | U/L | M < 35 · F < 30 |
  | GGT / gamma-GT (`ggt`) | U/L | M < 55 · F < 40 |
  | ALP / alkaline phosphatase (`alp`) | U/L | 40-120 |
  | Bilirubin, total (`bilirubin`) | µmol/L | < 17 |
  | Albumin (`albumin`) | g/L | 35-50 |
  | Creatinine (`creatinine`) | µmol/L | M 65-115 · F 50-95 |
  | eGFR (`egfr`) | mL/min/1.73 m² | > 90 |
  | Haemoglobin (`hemoglobin`) | mmol/L | M 8.5-10.5 · F 7.5-10.0 |
  | Haematocrit (`hematocrit`) | L/L | M 0.40-0.50 · F 0.35-0.45 |
  | Ferritin (`ferritin`) | µg/L | M 30-300 · F 15-200 |
  | LDL cholesterol (`ldl`) | mmol/L | < 3.0 |
  | HDL cholesterol (`hdl`) | mmol/L | M > 1.0 · F > 1.2 |
  | Triglycerides (`triglycerides`) | mmol/L | < 2.0 |
  | Glucose, fasting (`glucose`) | mmol/L | 4.0-6.0 |
  | HbA1c (`hba1c`) | mmol/mol | 20-42 |
  | Sodium (`sodium`) | mmol/L | 135-145 |
  | Potassium (`potassium`) | mmol/L | 3.5-5.0 |
  | Calcium, total (`calcium`) | mmol/L | 2.20-2.60 |
  | Vitamin D, 25-OH (`vitamin_d`) | nmol/L | 50-150 |
  | Vitamin B12 (`vitamin_b12`) | pmol/L | 150-600 |

  **Unit drop-down (fixed, no free text):** pmol/L, nmol/L, µmol/L, mmol/L, mmol/mol, U/L,
  mU/L, µg/L, ng/mL, pg/mL, mg/dL, ng/dL, µg/dL, g/L, g/dL, mg/L, mL/min/1.73 m², mL/min, %,
  L/L, 10⁹/L, 10¹²/L, fL, pg, mEq/L, IU/L, U/mL, ratio.

- **Consequences:** One central table to review and correct; adding a range is a data edit,
  not a migration. The values are accepted for v1.2 as general Dutch adult ranges; the P7
  wording pass still reads them for tone. Estradiol and similar HRT values deliberately carry
  **no target band** (D-012).

---

### D-026 — Lab graphs (deferred, recording kept graph-ready)
- **Date:** 2026-10-10 · **Status:** Accepted
- **Context:** Long-term we want to graph results (D-015), but v1.2 is recording and browsing.
- **Decision:** Defer the graph UI to a later v1.x phase and add **no chart dependency now**.
  Recording is shaped so a graph needs no rework: group by (catalogue key or custom text, unit)
  and plot `value` over `taken_at`, with a band drawn from D-025. The graph, when built, carries
  the D-012 “illustrative, not medical advice” label.
- **Consequences:** v1.2 ships list + add/edit/delete only; the graph is a pure UI addition on
  top of the same `level` rows.

---

### D-027 — Lab units are selectable; references always visible
- **Date:** 2026-10-10 · **Status:** Accepted
- **Context:** Device testing of P10 found the unit was locked to the catalogue's canonical
  unit, so a result the lab reports differently could not be entered. The two optional fields
  (lab, notes) looked identical, and the reference was not obvious.
- **Decision:** The unit drop-down is **always selectable**, defaulting to the analyte's usual
  unit. The reference appears as soon as an analyte is picked, labelled with the unit it
  applies to (`Reference (nmol/L): …`), together with a short “ranges vary by lab” disclaimer.
  Lab and Notes get distinct hints.
- **Consequences:** Refines D-024 (which locked the unit). The stored unit is whatever the
  user chose, so graphs still group by unit (D-026), and a reference shown for a unit other
  than the one picked is clearly labelled.

---

### D-028 — Labs are grouped per analyte (derived, no tracked-analyte table)
- **Date:** 2026-10-10 · **Status:** Accepted
- **Context:** A flat "all results" list made it unclear which result belonged to which test.
  The cleaner flow is: pick the test you track, see its results, log under it. An explicit
  `tracked_analyte` table (pre-adding empty tests) was considered.
- **Decision:** The Labs page is **grouped per analyte, derived from the `level` rows**
  (`Analytes::summarize`, one row per analyte, newest value/date wins, count kept). **No new
  table and no migration.** Tapping an analyte opens its results; Add there pre-sets and locks
  the analyte so a result cannot land under the wrong test. An analyte appears on its first
  result.
- **Consequences:** No "I plan to track X but have no values yet" entries — acceptable. The
  grouping lives in core and is unit-tested. The graph (D-026) reuses the same grouping, one
  unit at a time.

---

### D-029 — Lab graph: QML Canvas, one unit, bands only for clear ranges
- **Date:** 2026-10-10 · **Status:** Accepted
- **Context:** P11 adds a graph. Qt 5.6/Silica has no chart component and QtCharts is not in
  the allowed API set.
- **Decision:** Draw with a plain QML **`Canvas`** (line + points + axis), **no chart
  dependency**. One analyte, **one unit at a time**, with a unit switch when an analyte has
  more than one. A shaded band is drawn only where the catalogue range is a single two-sided
  numeric value in the catalogue unit (`kBands`, `AnalyteRef::hasBand`); sex-specific,
  one-sided and context-dependent ranges get no band, only the caption. The page carries the
  **“Illustrative only, not medical advice.”** line (D-012).
- **Consequences:** `kBands` duplicates part of the range text in `kAnalytes`; they must stay
  in sync (covered by a unit test that spot-checks TSH/ALP and rejects the sex-specific
  testosterone). No new library, so Harbour stays viable.

---

### D-030 — No unit conversion; the graph shows one unit at a time
- **Date:** 2026-10-10 · **Status:** Accepted
- **Context:** Labs can hold the same analyte in different units (D-027). Molar↔mass
  conversion is well-defined for a molecule (estradiol pmol/L↔pg/mL, testosterone
  nmol/L↔ng/dL, glucose mmol/L↔mg/dL), but enzyme activity (U/L) and some assays are
  lab/standard-specific, and a wrong factor is silently wrong health data.
- **Decision:** **Do not convert.** The graph plots **one unit at a time** and offers a unit
  switch when an analyte has more than one, with a note that results are not converted. The
  reference band still applies only to the catalogue unit (D-029). Unit stays stored as
  entered (D-024).
- **Consequences:** No conversion table to maintain or get wrong; mixed-unit results are shown
  side by side instead of merged, which is honest but means the user compares series
  themselves. Adding conversion later needs its own decision and a verified factor table.

---

### D-031 — Units per dose on the schedule, only for countable forms
- **Date:** 2026-10-11 · **Status:** Accepted
- **Context:** A schedule could not say "take 2 pills" — strength lives on the medication
  (one pill), so a dose of several units was not recordable. The v1 `regimen.dose` column
  existed but was never written by the UI (always 0).
- **Decision:** Reuse **`regimen.dose` as the number of units per dose** (0 = unspecified,
  treated as one). **No schema change, no migration.** The schedule page shows the field only
  when the medication's form is **Pill, Patch or Spray** ("Pills/Patches/Sprays per dose");
  decimals are allowed (half a pill). For other forms the count is saved as 0. The count shows
  in the dose text (`Pill · 2 × 2 mg`) and a taken dose logs `amount = strength × count`.
  It records what the user configures; there is no default other than one and no suggestion.
- **Consequences:** Forms were stored as translated text; D-032 makes them stable keys so the
  count field survives a language switch. Stock still decrements **one per taken dose** (D-023) — not changed here, since the
  stock unit may be "doses" rather than units.
- **Follow-up:** Changing a medication's form while it has schedules opens an "Update
  schedules?" dialog (Now / Later). Now opens the schedule (or the list when there are
  several); Later leaves them as they are. The stale count is never changed silently.

---

### D-032 — Forms and stock units are stored as stable keys
- **Date:** 2026-10-11 · **Status:** Accepted
- **Context:** `medication.form` and `stock.unit` held the translated text shown when the value
  was picked (`Pil`, `tabletter`, ...). After a language switch the pages could not find the
  stored value again: the method fell back to "Other", the units-per-dose field (D-031)
  disappeared, and the stock page reset its unit. The stock page also looked up the
  medication's unit instead of the stock unit.
- **Decision:** Store **English keys** (`Pill`…`Other`, `doses`…`g`) and translate only for
  display, through `Vocab` in core (`QT_TRANSLATE_NOOP`, context `Dollfin`, same pattern as
  `Analytes`). QML gets `[{key, label}]` from `app.formOptions()` / `app.stockUnitOptions()`.
  **Migration v3** rewrites existing nl/nb text to keys; **backup import** applies the same
  rewrite, because older backups hold translated text. Only values change; no column changes.
- **Consequences:** The legacy table is frozen at the 1.2 nl/nb translations; new languages
  never need an entry. Unknown text (anything not in the table) is kept as is and shown as
  is. Preset medication *names* are still stored as shown; they are almost all identical
  across languages, so they are left alone. Lets a later stock rule tell "doses" from counted
  units (D-031 follow-up).

---

### D-033 — Stock follows units per dose; the amount taken off is recorded
- **Date:** 2026-10-11 · **Status:** Accepted
- **Context:** D-023 took one off stock per taken dose. With units per dose (D-031) a schedule
  of 2 pills drifted when stock is counted in tablets. Undo has to give back the same amount,
  even if the schedule changed or was deleted in between.
- **Decision:** A taken dose takes off **1 when the stock unit is `doses`**, otherwise the
  schedule's **units per dose** (forms without a count stay at 1). Stock never goes below 0;
  only what was actually there is taken off. **Migration v4** adds a nullable
  `intake.stock_delta` holding what was taken off (0 for skipped or untracked stock). Undo
  (taken → skipped, delete) gives back exactly `stock_delta`; rows from before v4 (and
  older backups) have NULL and give back 1, as they took off 1. Logic lives in
  `StockLedger` (core, unit-tested).
- **Consequences:** Refines D-023. Untracked stock now records 0, so starting to track stock
  later and then undoing an earlier dose no longer adds a phantom unit. Partial-dose
  tracking for gels or injections (ml) is still out of scope.

---

## Open questions

Nothing is blocking. Deferred admin:

| # | Question | When |
|---|----------|------|
| Q3 | Name + email for the GPG release-signing key and OpenRepos account | P8 |

Resolved: Q1 → D-002 (Qt 5.6/Silica on Storeman); Q2 → D-014 (Bokmål only);
Q4 → D-006 (local calendar + `.ics` fallback).
