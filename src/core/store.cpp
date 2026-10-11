// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 lillelilje

#include "store.h"

#include "database.h"


#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace dollfin {
namespace {

QVariant nullString()
{
    return QVariant(QVariant::String);
}

QVariant textOrNull(const QString &value)
{
    return value.isEmpty() ? nullString() : QVariant(value);
}

QVariant dateOrNull(const QDate &date)
{
    return date.isValid() ? QVariant(date.toString(Qt::ISODate)) : nullString();
}

qint64 toEpoch(const QDateTime &dateTime)
{
    return dateTime.toUTC().toMSecsSinceEpoch() / 1000;
}

QDateTime fromEpoch(qint64 seconds)
{
    return QDateTime::fromMSecsSinceEpoch(seconds * 1000, Qt::UTC);
}

QString timesToJson(const QList<QTime> &times)
{
    QJsonArray array;
    for (const QTime &time : times)
        array.append(time.toString(QStringLiteral("HH:mm")));
    return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
}

QList<QTime> timesFromJson(const QString &json)
{
    QList<QTime> times;
    const QJsonArray array = QJsonDocument::fromJson(json.toUtf8()).array();
    for (const QJsonValue &value : array) {
        const QTime time = QTime::fromString(value.toString(), QStringLiteral("HH:mm"));
        if (time.isValid())
            times.append(time);
    }
    return times;
}

QString daysToCsv(const QList<int> &days)
{
    QStringList parts;
    for (int day : days)
        parts.append(QString::number(day));
    return parts.join(QLatin1Char(','));
}

QList<int> daysFromCsv(const QString &csv)
{
    QList<int> days;
    const QStringList parts = csv.split(QLatin1Char(','), QString::SkipEmptyParts);
    for (const QString &part : parts) {
        bool ok = false;
        const int day = part.toInt(&ok);
        if (ok && day >= 1 && day <= 7)
            days.append(day);
    }
    return days;
}

bool exec(QSqlQuery &query, QString *error)
{
    if (query.exec())
        return true;
    if (error)
        *error = query.lastError().text();
    return false;
}

} // namespace

Store::Store(Database &database)
    : m_database(database)
{
}

// --- Medications -----------------------------------------------------------

bool Store::addMedication(Medication &medication, QString *error)
{
    QSqlQuery query(m_database.handle());
    query.prepare(QStringLiteral(
        "INSERT INTO medication (name, form, strength, unit, notes, active) VALUES (?,?,?,?,?,?)"));
    query.addBindValue(medication.name);
    query.addBindValue(textOrNull(medication.form));
    query.addBindValue(medication.strength);
    query.addBindValue(textOrNull(medication.unit));
    query.addBindValue(textOrNull(medication.notes));
    query.addBindValue(medication.active ? 1 : 0);
    if (!exec(query, error))
        return false;
    medication.id = query.lastInsertId().toLongLong();
    return true;
}

bool Store::updateMedication(const Medication &medication, QString *error)
{
    QSqlQuery query(m_database.handle());
    query.prepare(QStringLiteral("UPDATE medication SET name=?, form=?, strength=?, unit=?,"
                                 " notes=?, active=? WHERE id=?"));
    query.addBindValue(medication.name);
    query.addBindValue(textOrNull(medication.form));
    query.addBindValue(medication.strength);
    query.addBindValue(textOrNull(medication.unit));
    query.addBindValue(textOrNull(medication.notes));
    query.addBindValue(medication.active ? 1 : 0);
    query.addBindValue(medication.id);
    return exec(query, error);
}

bool Store::removeMedication(qint64 id, QString *error)
{
    QSqlQuery query(m_database.handle());
    query.prepare(QStringLiteral("DELETE FROM medication WHERE id=?"));
    query.addBindValue(id);
    return exec(query, error);
}

Medication Store::medication(qint64 id, bool *ok) const
{
    Medication medication;
    QSqlQuery query(m_database.handle());
    query.prepare(QStringLiteral("SELECT id, name, form, strength, unit, notes, active"
                                 " FROM medication WHERE id=?"));
    query.addBindValue(id);
    const bool found = query.exec() && query.next();
    if (found) {
        medication.id = query.value(0).toLongLong();
        medication.name = query.value(1).toString();
        medication.form = query.value(2).toString();
        medication.strength = query.value(3).toDouble();
        medication.unit = query.value(4).toString();
        medication.notes = query.value(5).toString();
        medication.active = query.value(6).toInt() != 0;
    }
    if (ok)
        *ok = found;
    return medication;
}

QList<Medication> Store::medications(bool onlyActive) const
{
    QList<Medication> result;
    QSqlQuery query(m_database.handle());
    query.prepare(QStringLiteral("SELECT id, name, form, strength, unit, notes, active"
                                 " FROM medication") +
                  (onlyActive ? QStringLiteral(" WHERE active=1") : QString()) +
                  QStringLiteral(" ORDER BY name COLLATE NOCASE"));
    if (!query.exec())
        return result;
    while (query.next()) {
        Medication m;
        m.id = query.value(0).toLongLong();
        m.name = query.value(1).toString();
        m.form = query.value(2).toString();
        m.strength = query.value(3).toDouble();
        m.unit = query.value(4).toString();
        m.notes = query.value(5).toString();
        m.active = query.value(6).toInt() != 0;
        result.append(m);
    }
    return result;
}

// --- Regimens --------------------------------------------------------------

bool Store::addRegimen(Regimen &regimen, QString *error)
{
    QSqlQuery query(m_database.handle());
    query.prepare(QStringLiteral(
        "INSERT INTO regimen (medication_id, dose, unit, route, interval_days, days_of_week,"
        " times, start_date, end_date, active) VALUES (?,?,?,?,?,?,?,?,?,?)"));
    query.addBindValue(regimen.medicationId);
    query.addBindValue(regimen.dose);
    query.addBindValue(textOrNull(regimen.unit));
    query.addBindValue(textOrNull(regimen.route));
    query.addBindValue(regimen.intervalDays < 1 ? 1 : regimen.intervalDays);
    query.addBindValue(regimen.daysOfWeek.isEmpty() ? nullString() : QVariant(daysToCsv(regimen.daysOfWeek)));
    query.addBindValue(timesToJson(regimen.times));
    query.addBindValue(regimen.startDate.toString(Qt::ISODate));
    query.addBindValue(dateOrNull(regimen.endDate));
    query.addBindValue(regimen.active ? 1 : 0);
    if (!exec(query, error))
        return false;
    regimen.id = query.lastInsertId().toLongLong();
    return true;
}

bool Store::updateRegimen(const Regimen &regimen, QString *error)
{
    QSqlQuery query(m_database.handle());
    query.prepare(QStringLiteral(
        "UPDATE regimen SET medication_id=?, dose=?, unit=?, route=?, interval_days=?,"
        " days_of_week=?, times=?, start_date=?, end_date=?, active=? WHERE id=?"));
    query.addBindValue(regimen.medicationId);
    query.addBindValue(regimen.dose);
    query.addBindValue(textOrNull(regimen.unit));
    query.addBindValue(textOrNull(regimen.route));
    query.addBindValue(regimen.intervalDays < 1 ? 1 : regimen.intervalDays);
    query.addBindValue(regimen.daysOfWeek.isEmpty() ? nullString() : QVariant(daysToCsv(regimen.daysOfWeek)));
    query.addBindValue(timesToJson(regimen.times));
    query.addBindValue(regimen.startDate.toString(Qt::ISODate));
    query.addBindValue(dateOrNull(regimen.endDate));
    query.addBindValue(regimen.active ? 1 : 0);
    query.addBindValue(regimen.id);
    return exec(query, error);
}

bool Store::removeRegimen(qint64 id, QString *error)
{
    QSqlQuery query(m_database.handle());
    query.prepare(QStringLiteral("DELETE FROM regimen WHERE id=?"));
    query.addBindValue(id);
    return exec(query, error);
}

Regimen Store::regimen(qint64 id, bool *ok) const
{
    Regimen regimen;
    QSqlQuery query(m_database.handle());
    query.prepare(QStringLiteral(
        "SELECT id, medication_id, dose, unit, route, interval_days, days_of_week, times,"
        " start_date, end_date, active FROM regimen WHERE id=?"));
    query.addBindValue(id);
    const bool found = query.exec() && query.next();
    if (found) {
        regimen.id = query.value(0).toLongLong();
        regimen.medicationId = query.value(1).toLongLong();
        regimen.dose = query.value(2).toDouble();
        regimen.unit = query.value(3).toString();
        regimen.route = query.value(4).toString();
        regimen.intervalDays = query.value(5).toInt();
        regimen.daysOfWeek = daysFromCsv(query.value(6).toString());
        regimen.times = timesFromJson(query.value(7).toString());
        regimen.startDate = QDate::fromString(query.value(8).toString(), Qt::ISODate);
        regimen.endDate = QDate::fromString(query.value(9).toString(), Qt::ISODate);
        regimen.active = query.value(10).toInt() != 0;
    }
    if (ok)
        *ok = found;
    return regimen;
}

QList<Regimen> Store::regimens(qint64 medicationId, bool onlyActive) const
{
    QList<Regimen> result;
    QString sql = QStringLiteral(
        "SELECT id, medication_id, dose, unit, route, interval_days, days_of_week, times,"
        " start_date, end_date, active FROM regimen");
    QStringList conditions;
    if (medicationId >= 0)
        conditions.append(QStringLiteral("medication_id=") + QString::number(medicationId));
    if (onlyActive)
        conditions.append(QStringLiteral("active=1"));
    if (!conditions.isEmpty())
        sql += QStringLiteral(" WHERE ") + conditions.join(QStringLiteral(" AND "));
    sql += QStringLiteral(" ORDER BY id");

    QSqlQuery query(m_database.handle());
    if (!query.exec(sql))
        return result;
    while (query.next()) {
        Regimen r;
        r.id = query.value(0).toLongLong();
        r.medicationId = query.value(1).toLongLong();
        r.dose = query.value(2).toDouble();
        r.unit = query.value(3).toString();
        r.route = query.value(4).toString();
        r.intervalDays = query.value(5).toInt();
        r.daysOfWeek = daysFromCsv(query.value(6).toString());
        r.times = timesFromJson(query.value(7).toString());
        r.startDate = QDate::fromString(query.value(8).toString(), Qt::ISODate);
        r.endDate = QDate::fromString(query.value(9).toString(), Qt::ISODate);
        r.active = query.value(10).toInt() != 0;
        result.append(r);
    }
    return result;
}

// --- Intakes ---------------------------------------------------------------

bool Store::addIntake(Intake &intake, QString *error)
{
    QSqlQuery query(m_database.handle());
    query.prepare(QStringLiteral(
        "INSERT INTO intake (regimen_id, medication_id, scheduled_at, taken_at, amount, unit,"
        " status, site, notes, stock_delta) VALUES (?,?,?,?,?,?,?,?,?,?)"));
    query.addBindValue(intake.regimenId >= 0 ? QVariant(intake.regimenId) : QVariant(QVariant::LongLong));
    query.addBindValue(intake.medicationId);
    query.addBindValue(intake.scheduledAt.isValid() ? QVariant(toEpoch(intake.scheduledAt))
                                                    : QVariant(QVariant::LongLong));
    query.addBindValue(toEpoch(intake.takenAt));
    query.addBindValue(intake.amount);
    query.addBindValue(textOrNull(intake.unit));
    query.addBindValue(intake.status);
    query.addBindValue(textOrNull(intake.site));
    query.addBindValue(textOrNull(intake.notes));
    query.addBindValue(intake.stockDelta >= 0.0 ? QVariant(intake.stockDelta) : QVariant(QVariant::Double));
    if (!exec(query, error))
        return false;
    intake.id = query.lastInsertId().toLongLong();
    return true;
}

bool Store::updateIntake(const Intake &intake, QString *error)
{
    QSqlQuery query(m_database.handle());
    query.prepare(QStringLiteral(
        "UPDATE intake SET regimen_id=?, medication_id=?, scheduled_at=?, taken_at=?, amount=?,"
        " unit=?, status=?, site=?, notes=?, stock_delta=? WHERE id=?"));
    query.addBindValue(intake.regimenId >= 0 ? QVariant(intake.regimenId) : QVariant(QVariant::LongLong));
    query.addBindValue(intake.medicationId);
    query.addBindValue(intake.scheduledAt.isValid() ? QVariant(toEpoch(intake.scheduledAt))
                                                    : QVariant(QVariant::LongLong));
    query.addBindValue(toEpoch(intake.takenAt));
    query.addBindValue(intake.amount);
    query.addBindValue(textOrNull(intake.unit));
    query.addBindValue(intake.status);
    query.addBindValue(textOrNull(intake.site));
    query.addBindValue(textOrNull(intake.notes));
    query.addBindValue(intake.stockDelta >= 0.0 ? QVariant(intake.stockDelta) : QVariant(QVariant::Double));
    query.addBindValue(intake.id);
    return exec(query, error);
}

bool Store::removeIntake(qint64 id, QString *error)
{
    QSqlQuery query(m_database.handle());
    query.prepare(QStringLiteral("DELETE FROM intake WHERE id=?"));
    query.addBindValue(id);
    return exec(query, error);
}

QList<Intake> Store::intakes(qint64 medicationId) const
{
    QList<Intake> result;
    QSqlQuery query(m_database.handle());
    query.prepare(QStringLiteral(
        "SELECT id, regimen_id, medication_id, scheduled_at, taken_at, amount, unit, status,"
        " site, notes, stock_delta FROM intake") +
        (medicationId >= 0 ? QStringLiteral(" WHERE medication_id=?") : QString()) +
        QStringLiteral(" ORDER BY taken_at DESC"));
    if (medicationId >= 0)
        query.addBindValue(medicationId);
    if (!query.exec())
        return result;
    while (query.next()) {
        Intake i;
        i.id = query.value(0).toLongLong();
        i.regimenId = query.value(1).isNull() ? -1 : query.value(1).toLongLong();
        i.medicationId = query.value(2).toLongLong();
        if (!query.value(3).isNull())
            i.scheduledAt = fromEpoch(query.value(3).toLongLong());
        i.takenAt = fromEpoch(query.value(4).toLongLong());
        i.amount = query.value(5).toDouble();
        i.unit = query.value(6).toString();
        i.status = query.value(7).toString();
        i.site = query.value(8).toString();
        i.notes = query.value(9).toString();
        i.stockDelta = query.value(10).isNull() ? -1.0 : query.value(10).toDouble();
        result.append(i);
    }
    return result;
}

// --- Stock -----------------------------------------------------------------

bool Store::setStock(const Stock &stock, QString *error)
{
    QSqlQuery query(m_database.handle());
    query.prepare(QStringLiteral(
        "INSERT OR REPLACE INTO stock (medication_id, quantity, unit, low_threshold, last_refill,"
        " expires, source) VALUES (?,?,?,?,?,?,?)"));
    query.addBindValue(stock.medicationId);
    query.addBindValue(stock.quantity);
    query.addBindValue(textOrNull(stock.unit));
    query.addBindValue(stock.lowThreshold);
    query.addBindValue(dateOrNull(stock.lastRefill));
    query.addBindValue(dateOrNull(stock.expires));
    query.addBindValue(textOrNull(stock.source));
    return exec(query, error);
}

Stock Store::stock(qint64 medicationId, bool *ok) const
{
    Stock stock;
    QSqlQuery query(m_database.handle());
    query.prepare(QStringLiteral(
        "SELECT medication_id, quantity, unit, low_threshold, last_refill, expires, source"
        " FROM stock WHERE medication_id=?"));
    query.addBindValue(medicationId);
    const bool found = query.exec() && query.next();
    if (found) {
        stock.medicationId = query.value(0).toLongLong();
        stock.quantity = query.value(1).toDouble();
        stock.unit = query.value(2).toString();
        stock.lowThreshold = query.value(3).toDouble();
        stock.lastRefill = QDate::fromString(query.value(4).toString(), Qt::ISODate);
        stock.expires = QDate::fromString(query.value(5).toString(), Qt::ISODate);
        stock.source = query.value(6).toString();
    }
    if (ok)
        *ok = found;
    return stock;
}

// --- Levels ----------------------------------------------------------------

bool Store::addLevel(Level &level, QString *error)
{
    QSqlQuery query(m_database.handle());
    query.prepare(QStringLiteral(
        "INSERT INTO level (taken_at, analyte, value, unit, lab, notes) VALUES (?,?,?,?,?,?)"));
    query.addBindValue(toEpoch(level.takenAt));
    query.addBindValue(level.analyte);
    query.addBindValue(level.value);
    query.addBindValue(textOrNull(level.unit));
    query.addBindValue(textOrNull(level.lab));
    query.addBindValue(textOrNull(level.notes));
    if (!exec(query, error))
        return false;
    level.id = query.lastInsertId().toLongLong();
    return true;
}

bool Store::updateLevel(const Level &level, QString *error)
{
    QSqlQuery query(m_database.handle());
    query.prepare(QStringLiteral("UPDATE level SET taken_at=?, analyte=?, value=?, unit=?,"
                                 " lab=?, notes=? WHERE id=?"));
    query.addBindValue(toEpoch(level.takenAt));
    query.addBindValue(level.analyte);
    query.addBindValue(level.value);
    query.addBindValue(textOrNull(level.unit));
    query.addBindValue(textOrNull(level.lab));
    query.addBindValue(textOrNull(level.notes));
    query.addBindValue(level.id);
    return exec(query, error);
}

bool Store::removeLevel(qint64 id, QString *error)
{
    QSqlQuery query(m_database.handle());
    query.prepare(QStringLiteral("DELETE FROM level WHERE id=?"));
    query.addBindValue(id);
    return exec(query, error);
}

Level Store::level(qint64 id, bool *ok) const
{
    Level level;
    QSqlQuery query(m_database.handle());
    query.prepare(QStringLiteral("SELECT id, taken_at, analyte, value, unit, lab, notes"
                                 " FROM level WHERE id=?"));
    query.addBindValue(id);
    const bool found = query.exec() && query.next();
    if (found) {
        level.id = query.value(0).toLongLong();
        level.takenAt = fromEpoch(query.value(1).toLongLong());
        level.analyte = query.value(2).toString();
        level.value = query.value(3).toDouble();
        level.unit = query.value(4).toString();
        level.lab = query.value(5).toString();
        level.notes = query.value(6).toString();
    }
    if (ok)
        *ok = found;
    return level;
}

QList<Level> Store::levels() const
{
    QList<Level> result;
    QSqlQuery query(m_database.handle());
    if (!query.exec(QStringLiteral("SELECT id, taken_at, analyte, value, unit, lab, notes"
                                   " FROM level ORDER BY taken_at DESC")))
        return result;
    while (query.next()) {
        Level l;
        l.id = query.value(0).toLongLong();
        l.takenAt = fromEpoch(query.value(1).toLongLong());
        l.analyte = query.value(2).toString();
        l.value = query.value(3).toDouble();
        l.unit = query.value(4).toString();
        l.lab = query.value(5).toString();
        l.notes = query.value(6).toString();
        result.append(l);
    }
    return result;
}

} // namespace dollfin
