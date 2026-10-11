// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 lillelilje
//
// Minimal zero-dependency test harness (QtTest is not packaged in the
// Sailfish target). Run with scripts/run-tests.sh, or from an SDK build shell:
//   cd tests/tst_core && qmake && make && ./tst_core

#include "core/analytes.h"
#include "core/backup.h"
#include "core/database.h"
#include "core/ical.h"
#include "core/scheduleengine.h"
#include "core/store.h"

#include <QCoreApplication>
#include <QDate>
#include <QDebug>
#include <QDir>
#include <QSet>
#include <QTemporaryDir>
#include <QTime>

#include <ctime>
#include <cstdio>

using namespace dollfin;

static int g_checks = 0;
static int g_failures = 0;

static void reportFailure(const char *file, int line, const char *expression, const QString &detail = QString())
{
    ++g_failures;
    std::fprintf(stderr, "FAIL %s:%d: %s%s%s\n", file, line, expression,
                 detail.isEmpty() ? "" : " -- ", detail.toUtf8().constData());
    std::fflush(stderr);
}

#define CHECK(cond)                                                        \
    do {                                                                   \
        ++g_checks;                                                        \
        if (!(cond))                                                       \
            reportFailure(__FILE__, __LINE__, #cond);                      \
    } while (0)

#define CHECK_EQ(actual, expected)                                                    \
    do {                                                                              \
        ++g_checks;                                                                   \
        const auto a_ = (actual);                                                     \
        const auto e_ = (expected);                                                   \
        if (!(a_ == e_)) {                                                            \
            QString detail;                                                           \
            QDebug(&detail) << "got" << a_ << "expected" << e_;                     \
            reportFailure(__FILE__, __LINE__, #actual " == " #expected, detail);      \
        }                                                                             \
    } while (0)

static void useTimezone(const char *tz)
{
    qputenv("TZ", tz);
    tzset();
}

static QDate med(const char *iso)
{
    return QDate::fromString(QString::fromLatin1(iso), Qt::ISODate);
}

static QDateTime utc(const char *iso)
{
    return QDateTime(QDate::fromString(QString::fromLatin1(iso).left(10), Qt::ISODate),
                     QTime::fromString(QString::fromLatin1(iso).mid(11, 5), QStringLiteral("HH:mm")),
                     Qt::UTC);
}

static void testSchemaAndMigration()
{
    QTemporaryDir dir;
    Database db;
    QString error;
    const bool openOk = db.open(dir.path() + "/schema.db", &error);
    if (!openOk)
        std::fprintf(stderr, "open failed: %s\n", error.toUtf8().constData());
    CHECK(openOk);
    CHECK_EQ(db.schemaVersion(), 2);

    // Re-opening an existing database must not re-run migrations.
    db.close();
    Database db2;
    CHECK(db2.open(dir.path() + "/schema.db", &error));
    CHECK_EQ(db2.schemaVersion(), 2);
}

static void testMedicationCrud()
{
    QTemporaryDir dir;
    Database db;
    CHECK(db.open(dir.path() + "/med.db"));
    Store store(db);

    Medication m;
    m.name = QStringLiteral("Estradiol hemihydrate (compounded) 2 mg"); // free text, D-017
    m.form = QStringLiteral("tablet");
    m.strength = 2.0;
    m.unit = QStringLiteral("mg");
    CHECK(store.addMedication(m));
    CHECK(m.id > 0);

    Medication other;
    other.name = QStringLiteral("Testosterone gel");
    CHECK(store.addMedication(other));

    CHECK_EQ(store.medications().size(), 2);

    m.notes = QStringLiteral("take sublingually");
    CHECK(store.updateMedication(m));

    bool ok = false;
    const Medication fetched = store.medication(m.id, &ok);
    CHECK(ok);
    CHECK(fetched.notes == QStringLiteral("take sublingually"));
    CHECK_EQ(fetched.strength, 2.0);

    CHECK(store.removeMedication(other.id));
    CHECK_EQ(store.medications().size(), 1);
    store.medication(other.id, &ok);
    CHECK(!ok);
}

static void testRegimenCrudAndCascade()
{
    QTemporaryDir dir;
    Database db;
    CHECK(db.open(dir.path() + "/reg.db"));
    Store store(db);

    Medication m;
    m.name = QStringLiteral("Estradiol valerate");
    CHECK(store.addMedication(m));

    Regimen r;
    r.medicationId = m.id;
    r.dose = 5.0;
    r.unit = QStringLiteral("mg");
    r.route = QStringLiteral("injection");
    r.intervalDays = 5;
    r.times = QList<QTime>() << QTime(8, 0) << QTime(20, 0);
    r.startDate = med("2026-01-01");
    CHECK(store.addRegimen(r));
    CHECK(r.id > 0);

    bool ok = false;
    const Regimen fetched = store.regimen(r.id, &ok);
    CHECK(ok);
    CHECK_EQ(fetched.times.size(), 2);
    CHECK_EQ(fetched.intervalDays, 5);
    CHECK_EQ(fetched.route, QStringLiteral("injection"));

    // Units per dose (D-031): stored on the schedule, unspecified means one.
    CHECK_EQ(fetched.dose, 5.0);
    CHECK_EQ(unitsPerDose(fetched), 5.0);
    Regimen halfPill = fetched;
    halfPill.dose = 0.5;
    CHECK(store.updateRegimen(halfPill));
    CHECK_EQ(store.regimen(r.id, &ok).dose, 0.5);
    CHECK_EQ(unitsPerDose(Regimen()), 1.0);

    // Weekly regimen with explicit weekdays.
    Regimen weekly;
    weekly.medicationId = m.id;
    weekly.times = QList<QTime>() << QTime(9, 0);
    weekly.daysOfWeek = QList<int>() << 1 << 3 << 5;
    weekly.startDate = med("2026-01-05");
    CHECK(store.addRegimen(weekly));
    const Regimen w = store.regimen(weekly.id, &ok);
    CHECK(ok);
    CHECK_EQ(w.daysOfWeek.size(), 3);

    // Removing the medication cascades to its regimens.
    CHECK_EQ(store.regimens(m.id).size(), 2);
    CHECK(store.removeMedication(m.id));
    CHECK_EQ(store.regimens().size(), 0);
}

static void testIntakeCrud()
{
    QTemporaryDir dir;
    Database db;
    QString error;
    const bool openOk = db.open(dir.path() + "/intake.db", &error);
    if (!openOk)
        std::fprintf(stderr, "intake open failed: %s\n", error.toUtf8().constData());
    CHECK(openOk);
    Store store(db);

    Medication m;
    m.name = QStringLiteral("Progesterone");
    CHECK(store.addMedication(m));

    Intake i;
    i.medicationId = m.id;
    i.regimenId = -1;
    i.takenAt = utc("2026-02-01T21:00");
    i.amount = 100.0;
    i.unit = QStringLiteral("mg");
    i.status = QStringLiteral("taken");
    CHECK(store.addIntake(i));
    CHECK(i.id > 0);

    CHECK_EQ(store.intakes().size(), 1);
    const QList<Intake> list = store.intakes(m.id);
    CHECK_EQ(list.size(), 1);
    if (!list.isEmpty()) {
        const Intake f = list.first();
        CHECK(f.status == QStringLiteral("taken"));
        CHECK_EQ(f.amount, 100.0);
        CHECK(f.takenAt.toUTC().toString(Qt::ISODate) == QStringLiteral("2026-02-01T21:00:00Z"));
    }

    Intake skipped;
    skipped.medicationId = m.id;
    skipped.regimenId = -1;
    skipped.takenAt = utc("2026-02-02T21:00");
    skipped.status = QStringLiteral("skipped");
    CHECK(store.addIntake(skipped));
    CHECK_EQ(store.intakes().size(), 2);

    CHECK(store.removeIntake(i.id));
    CHECK_EQ(store.intakes().size(), 1);
}

static Regimen dailyRegimen()
{
    Regimen r;
    r.active = true;
    r.intervalDays = 1;
    r.times = QList<QTime>() << QTime(8, 0) << QTime(20, 0);
    r.startDate = med("2026-01-01");
    return r;
}

static void testScheduleDaily()
{
    const Regimen r = dailyRegimen();
    const QList<QDateTime> occ = ScheduleEngine::occurrences(r, utc("2026-01-01T00:00"), utc("2026-01-04T00:00"));
    CHECK_EQ(occ.size(), 6);
    // Sorted ascending.
    CHECK(occ.first() < occ.last());
}

static void testScheduleInterval()
{
    Regimen r = dailyRegimen();
    r.intervalDays = 3;
    r.times = QList<QTime>() << QTime(9, 0);
    const QList<QDateTime> occ = ScheduleEngine::occurrences(r, utc("2026-01-01T00:00"), utc("2026-01-11T00:00"));
    CHECK_EQ(occ.size(), 4); // Jan 1, 4, 7, 10
}

static void testScheduleWeekdays()
{
    Regimen r;
    r.active = true;
    r.times = QList<QTime>() << QTime(10, 0);
    r.daysOfWeek = QList<int>() << 1 << 3 << 5; // Mon, Wed, Fri
    r.startDate = med("2026-01-05");            // a Monday
    const QList<QDateTime> occ = ScheduleEngine::occurrences(r, utc("2026-01-05T00:00"), utc("2026-01-12T00:00"));
    CHECK_EQ(occ.size(), 3); // Mon 5, Wed 7, Fri 9
}

static void testScheduleEndDateAndInactive()
{
    Regimen r = dailyRegimen();
    r.times = QList<QTime>() << QTime(8, 0);
    r.endDate = med("2026-01-03");
    CHECK_EQ(ScheduleEngine::occurrences(r, utc("2026-01-01T00:00"), utc("2026-01-31T00:00")).size(), 3);

    r.active = false;
    CHECK_EQ(ScheduleEngine::occurrences(r, utc("2026-01-01T00:00"), utc("2026-01-31T00:00")).size(), 0);
}

static void testScheduleStartInFuture()
{
    Regimen r = dailyRegimen();
    r.startDate = med("2026-06-01");
    CHECK_EQ(ScheduleEngine::occurrences(r, utc("2026-01-01T00:00"), utc("2026-02-01T00:00")).size(), 0);
}

static void testScheduleMonthYearBoundary()
{
    Regimen r = dailyRegimen();
    r.times = QList<QTime>() << QTime(23, 0);
    r.startDate = med("2026-12-31");
    const QList<QDateTime> occ = ScheduleEngine::occurrences(r, utc("2026-12-31T00:00"), utc("2027-01-03T00:00"));
    CHECK_EQ(occ.size(), 3); // Dec 31, Jan 1, Jan 2
}

static void testScheduleDst()
{
    useTimezone("Europe/Berlin");
    Regimen r = dailyRegimen();
    r.times = QList<QTime>() << QTime(12, 0);
    r.startDate = med("2026-03-28");
    const QList<QDateTime> occ = ScheduleEngine::occurrences(r, utc("2026-03-28T00:00"), utc("2026-03-31T00:00"));
    CHECK_EQ(occ.size(), 3); // DST spring-forward on 2026-03-29 must not drop or duplicate a day
    if (occ.size() == 3) {
        // 12:00 CET = 11:00 UTC; after the change 12:00 CEST = 10:00 UTC.
        CHECK_EQ(occ[0].toUTC().time().hour(), 11);
        CHECK_EQ(occ[2].toUTC().time().hour(), 10);
    }
    useTimezone("UTC");
}

static void testFirstOccurrenceAfter()
{
    const Regimen r = dailyRegimen();
    const QDateTime next = ScheduleEngine::firstOccurrenceAfter(r, utc("2026-01-02T09:00"));
    CHECK(next.isValid());
    CHECK(next > utc("2026-01-02T09:00"));
    CHECK_EQ(next.toUTC().toString(Qt::ISODate), QStringLiteral("2026-01-02T20:00:00Z"));

    Regimen paused = r;
    paused.active = false;
    CHECK(!ScheduleEngine::firstOccurrenceAfter(paused, utc("2026-01-02T09:00")).isValid());
}

static void testAnalyteCatalog()
{
    const QList<AnalyteRef> catalog = Analytes::catalog();
    CHECK(!catalog.isEmpty());

    const QStringList units = Analytes::units();
    QSet<QString> seen;
    for (const AnalyteRef &analyte : catalog) {
        CHECK(!analyte.key.isEmpty());
        CHECK(!analyte.label.isEmpty());
        CHECK(!analyte.unit.isEmpty());
        CHECK(!analyte.reference.isEmpty());
        CHECK(units.contains(analyte.unit));
        CHECK(!seen.contains(analyte.key)); // keys unique
        seen.insert(analyte.key);
        CHECK_EQ(Analytes::label(analyte.key), analyte.label);
    }

    CHECK(Analytes::find(QStringLiteral("estradiol")) != nullptr);
    CHECK(Analytes::find(QStringLiteral("no_such_analyte")) == nullptr);

    // Graph bands only for clear two-sided ranges (D-029).
    const AnalyteRef *tsh = Analytes::find(QStringLiteral("tsh"));
    CHECK(tsh && tsh->hasBand);
    if (tsh) {
        CHECK_EQ(tsh->bandLow, 0.4);
        CHECK_EQ(tsh->bandHigh, 4.0);
    }
    const AnalyteRef *testosterone = Analytes::find(QStringLiteral("testosterone"));
    CHECK(testosterone && !testosterone->hasBand); // sex-specific, no band
    // Unknown keys fall back to the stored text (custom analytes).
    CHECK_EQ(Analytes::label(QStringLiteral("My custom test")),
             QStringLiteral("My custom test"));
}

static void testAnalyteSummary()
{
    Level older;
    older.analyte = QStringLiteral("estradiol");
    older.value = 100;
    older.unit = QStringLiteral("pmol/L");
    older.takenAt = utc("2026-03-01T00:00");

    Level newer;
    newer.analyte = QStringLiteral("estradiol");
    newer.value = 150;
    newer.unit = QStringLiteral("pmol/L");
    newer.takenAt = utc("2026-04-01T00:00");

    Level custom;
    custom.analyte = QStringLiteral("My test");
    custom.value = 5;
    custom.unit = QStringLiteral("U/L");
    custom.takenAt = utc("2026-02-01T00:00");

    // Deliberately unsorted input: the newest taken_at must win.
    const QList<AnalyteSummary> s =
        Analytes::summarize(QList<Level>() << custom << older << newer);
    CHECK_EQ(s.size(), 2);
    CHECK_EQ(s.first().key, QStringLiteral("estradiol")); // newest date first
    CHECK_EQ(s.first().count, 2);
    CHECK_EQ(s.first().value, 150.0);
    CHECK_EQ(s.first().date, QDate(2026, 4, 1));
    CHECK_EQ(s.first().referenceUnit, QStringLiteral("pmol/L"));
    CHECK(!s.first().custom);
    CHECK(s.at(1).custom);
    CHECK_EQ(s.at(1).label, QStringLiteral("My test"));
    CHECK_EQ(s.at(1).count, 1);
    CHECK(s.at(1).reference.isEmpty());

    Level tshLevel;
    tshLevel.analyte = QStringLiteral("tsh");
    tshLevel.value = 2.0;
    tshLevel.unit = QStringLiteral("mU/L");
    tshLevel.takenAt = utc("2026-05-01T00:00");
    const QList<AnalyteSummary> t = Analytes::summarize(QList<Level>() << tshLevel);
    CHECK_EQ(t.size(), 1);
    CHECK(t.first().hasBand);
    CHECK_EQ(t.first().bandLow, 0.4);
    CHECK_EQ(t.first().bandHigh, 4.0);
}

static void testLevelCrud()
{
    QTemporaryDir dir;
    Database db;
    CHECK(db.open(dir.path() + "/levels.db"));
    Store store(db);

    Level l;
    l.takenAt = utc("2026-03-01T00:00");
    l.analyte = QStringLiteral("estradiol");
    l.value = 123.4;
    l.unit = QStringLiteral("pmol/L");
    l.lab = QStringLiteral("AMC");
    l.notes = QStringLiteral("morning");
    CHECK(store.addLevel(l));
    CHECK(l.id > 0);

    bool ok = false;
    Level read = store.level(l.id, &ok);
    CHECK(ok);
    CHECK_EQ(read.analyte, QStringLiteral("estradiol"));
    CHECK_EQ(read.value, 123.4);
    CHECK_EQ(read.unit, QStringLiteral("pmol/L"));
    CHECK_EQ(read.lab, QStringLiteral("AMC"));

    read.value = 150.0;
    read.analyte = QStringLiteral("My custom test");
    CHECK(store.updateLevel(read));
    read = store.level(l.id, &ok);
    CHECK(ok);
    CHECK_EQ(read.value, 150.0);
    CHECK_EQ(read.analyte, QStringLiteral("My custom test"));

    Level second;
    second.takenAt = utc("2026-04-01T00:00");
    second.analyte = QStringLiteral("alt");
    second.value = 20;
    CHECK(store.addLevel(second));
    const QList<Level> all = store.levels();
    CHECK_EQ(all.size(), 2);
    CHECK_EQ(all.first().analyte, QStringLiteral("alt")); // newest first

    CHECK(store.removeLevel(l.id));
    CHECK_EQ(store.levels().size(), 1);
}

static void testIcalBuilder()
{
    Regimen r;
    r.id = 7;
    r.active = true;
    r.intervalDays = 3;
    r.times = QList<QTime>() << QTime(9, 0);
    r.startDate = med("2026-01-05"); // a Monday

    const QString everyThree = ICal::event(r, QStringLiteral("Estradiol"), QTime(9, 0));
    CHECK(everyThree.contains(QStringLiteral("UID:dollfin-7-0900@dollfin")));
    CHECK(everyThree.contains(QStringLiteral("RRULE:FREQ=DAILY;INTERVAL=3")));
    CHECK(everyThree.contains(QStringLiteral("DTSTART:20260105T090000")));
    CHECK(everyThree.contains(QStringLiteral("DURATION:PT1M")));      // D-021
    CHECK(everyThree.contains(QStringLiteral("TRIGGER:PT0S")));        // at event start
    CHECK(everyThree.contains(QStringLiteral("SUMMARY:Estradiol")));
    CHECK(everyThree.contains(QStringLiteral("\r\n")));

    // Weekly: start on a Tuesday, first dosing day must be the Wednesday.
    Regimen weekly;
    weekly.id = 8;
    weekly.active = true;
    weekly.daysOfWeek = QList<int>() << 1 << 3 << 5; // Mon, Wed, Fri
    weekly.times = QList<QTime>() << QTime(20, 0);
    weekly.startDate = med("2026-01-06");
    const QString weeklyEvent = ICal::event(weekly, QStringLiteral("Gel"), QTime(20, 0));
    CHECK(weeklyEvent.contains(QStringLiteral("RRULE:FREQ=WEEKLY;BYDAY=MO,WE,FR")));
    CHECK(weeklyEvent.contains(QStringLiteral("DTSTART:20260107T200000")));

    // Daily (interval 1) has no INTERVAL.
    Regimen daily;
    daily.id = 9;
    daily.active = true;
    daily.intervalDays = 1;
    daily.times = QList<QTime>() << QTime(8, 0);
    daily.startDate = med("2026-01-01");
    CHECK(ICal::event(daily, QStringLiteral("X"), QTime(8, 0))
              .contains(QStringLiteral("RRULE:FREQ=DAILY\r\n")));

    const QString calendar = ICal::wrap(QStringList() << everyThree << weeklyEvent);
    CHECK(calendar.startsWith(QStringLiteral("BEGIN:VCALENDAR\r\n")));
    CHECK(calendar.endsWith(QStringLiteral("END:VCALENDAR\r\n")));
}

static void testStockAndBackup()
{
    QTemporaryDir dir;
    Database db;
    CHECK(db.open(dir.path() + "/backup.db"));
    Store store(db);

    Medication m;
    m.name = QStringLiteral("Testosterone gel");
    m.form = QStringLiteral("Gel");
    m.strength = 16.2;
    m.unit = QStringLiteral("mg");
    CHECK(store.addMedication(m));

    Regimen r;
    r.medicationId = m.id;
    r.times = QList<QTime>() << QTime(8, 0);
    r.startDate = med("2026-01-01");
    CHECK(store.addRegimen(r));

    Intake i;
    i.medicationId = m.id;
    i.regimenId = r.id;
    i.takenAt = utc("2026-02-01T08:00");
    i.status = QStringLiteral("taken");
    CHECK(store.addIntake(i));

    Stock stock;
    stock.medicationId = m.id;
    stock.quantity = 10;
    stock.unit = QStringLiteral("doses");
    stock.lowThreshold = 2;
    stock.source = QStringLiteral("Pharmacy");
    CHECK(store.setStock(stock));

    Level lab;
    lab.takenAt = utc("2026-02-10T00:00");
    lab.analyte = QStringLiteral("testosterone");
    lab.value = 18.5;
    lab.unit = QStringLiteral("nmol/L");
    CHECK(store.addLevel(lab));

    bool ok = false;
    const Stock readBack = store.stock(m.id, &ok);
    CHECK(ok);
    CHECK_EQ(readBack.quantity, 10.0);
    CHECK(readBack.source == QStringLiteral("Pharmacy"));

    // Export, then import into a fresh database — everything must round-trip.
    const QString json = Backup::toJson(db);
    CHECK(json.contains(QStringLiteral("dollfin-backup")));
    CHECK(json.contains(QStringLiteral("Pharmacy")));

    Database restored;
    CHECK(restored.open(dir.path() + "/restored.db"));
    QString error;
    const bool imported = Backup::fromJson(restored, json, &error);
    if (!imported)
        std::fprintf(stderr, "import error: %s\n", error.toUtf8().constData());
    CHECK(imported);

    Store restoredStore(restored);
    CHECK_EQ(restoredStore.medications().size(), 1);
    CHECK_EQ(restoredStore.regimens().size(), 1);
    CHECK_EQ(restoredStore.intakes().size(), 1);
    CHECK_EQ(restoredStore.levels().size(), 1);
    if (!restoredStore.levels().isEmpty())
        CHECK_EQ(restoredStore.levels().first().value, 18.5);
    const QList<Medication> meds = restoredStore.medications();
    if (!meds.isEmpty()) {
        const Stock restoredStock = restoredStore.stock(meds.first().id, &ok);
        CHECK(ok);
        CHECK_EQ(restoredStock.quantity, 10.0);
        CHECK(restoredStock.source == QStringLiteral("Pharmacy"));
    }

    // Importing a non-backup must be refused.
    CHECK(!Backup::fromJson(restored, QStringLiteral("{}"), &error));
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    useTimezone("UTC");

    testSchemaAndMigration();
    testMedicationCrud();
    testRegimenCrudAndCascade();
    testIntakeCrud();
    testScheduleDaily();
    testScheduleInterval();
    testScheduleWeekdays();
    testScheduleEndDateAndInactive();
    testScheduleStartInFuture();
    testScheduleMonthYearBoundary();
    testScheduleDst();
    testFirstOccurrenceAfter();
    testAnalyteCatalog();
    testAnalyteSummary();
    testLevelCrud();
    testIcalBuilder();
    testStockAndBackup();

    std::fprintf(stderr, "%d checks, %d failure(s)\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
