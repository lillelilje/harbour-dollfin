// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 lillelilje

#include "database.h"

#include "vocab.h"

#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace dollfin {

Database::Database()
    : m_connectionName(QStringLiteral("dollfin-%1").arg(reinterpret_cast<quintptr>(this), 0, 16))
{
}

Database::~Database()
{
    close();
}

bool Database::open(const QString &filePath, QString *error)
{
    close();
    m_filePath = filePath;

    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_db.setDatabaseName(filePath);
    if (!m_db.open()) {
        if (error)
            *error = m_db.lastError().text();
        QSqlDatabase::removeDatabase(m_connectionName);
        m_db = QSqlDatabase();
        return false;
    }

    // Enforce the foreign keys declared in the schema.
    if (!run(QStringLiteral("PRAGMA foreign_keys = ON"), error)) {
        close();
        return false;
    }

    return migrate(error);
}

void Database::close()
{
    if (m_db.isValid()) {
        m_db.close();
        m_db = QSqlDatabase();
    }
    if (!m_connectionName.isEmpty() && QSqlDatabase::contains(m_connectionName))
        QSqlDatabase::removeDatabase(m_connectionName);
}

bool Database::isOpen() const
{
    return m_db.isOpen();
}

bool Database::run(const QString &sql, QString *error)
{
    QSqlQuery query(m_db);
    if (!query.exec(sql)) {
        if (error)
            *error = query.lastError().text() + QStringLiteral(" [") + sql + QLatin1Char(']');
        return false;
    }
    return true;
}

int Database::schemaVersion() const
{
    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral("SELECT MAX(version) FROM schema_version")) || !query.next())
        return 0;
    return query.value(0).toInt();
}

bool Database::migrate(QString *error)
{
    if (!run(QStringLiteral("CREATE TABLE IF NOT EXISTS schema_version ("
                            "version INTEGER NOT NULL, applied_at TEXT NOT NULL)"),
             error))
        return false;

    const int current = schemaVersion();
    if (current >= SchemaVersion)
        return true;

    if (!m_db.transaction()) {
        if (error)
            *error = m_db.lastError().text();
        return false;
    }

    if (current < 1) {
        // Raw string literals: one element per statement, no concatenation pitfalls.
        static const char *const kSchema[] = {
            R"(CREATE TABLE medication (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT NOT NULL,
                form TEXT,
                strength REAL NOT NULL DEFAULT 0,
                unit TEXT,
                notes TEXT,
                active INTEGER NOT NULL DEFAULT 1))",

            R"(CREATE TABLE regimen (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                medication_id INTEGER NOT NULL REFERENCES medication(id) ON DELETE CASCADE,
                dose REAL NOT NULL DEFAULT 0,
                unit TEXT,
                route TEXT,
                interval_days INTEGER NOT NULL DEFAULT 1,
                days_of_week TEXT,
                times TEXT NOT NULL,
                start_date TEXT NOT NULL,
                end_date TEXT,
                active INTEGER NOT NULL DEFAULT 1))",

            R"(CREATE TABLE intake (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                regimen_id INTEGER REFERENCES regimen(id) ON DELETE SET NULL,
                medication_id INTEGER NOT NULL REFERENCES medication(id) ON DELETE CASCADE,
                scheduled_at INTEGER,
                taken_at INTEGER NOT NULL,
                amount REAL NOT NULL DEFAULT 0,
                unit TEXT,
                status TEXT NOT NULL,
                site TEXT,
                notes TEXT))",

            R"(CREATE TABLE stock (
                medication_id INTEGER PRIMARY KEY REFERENCES medication(id) ON DELETE CASCADE,
                quantity REAL NOT NULL DEFAULT 0,
                unit TEXT,
                low_threshold REAL NOT NULL DEFAULT 0,
                last_refill TEXT,
                expires TEXT))",

            R"(CREATE TABLE level (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                taken_at INTEGER NOT NULL,
                analyte TEXT NOT NULL,
                value REAL NOT NULL DEFAULT 0,
                unit TEXT,
                lab TEXT,
                notes TEXT))",

            R"(CREATE INDEX idx_regimen_medication ON regimen(medication_id))",
            R"(CREATE INDEX idx_intake_regimen ON intake(regimen_id))",
            R"(CREATE INDEX idx_intake_medication ON intake(medication_id))",
            R"(CREATE INDEX idx_intake_scheduled ON intake(scheduled_at))",
        };
        for (const char *sql : kSchema) {
            if (!run(QString::fromLatin1(sql), error)) {
                m_db.rollback();
                return false;
            }
        }
    }

    if (current < 2) {
        // Stock source: where the supply came from (prescriber, pharmacy, ...).
        if (!run(QStringLiteral("ALTER TABLE stock ADD COLUMN source TEXT"), error)) {
            m_db.rollback();
            return false;
        }
    }

    if (current < 3) {
        // Forms and stock units become stable keys instead of translated text (D-032).
        if (!Vocab::normalizeStored(m_db, error)) {
            m_db.rollback();
            return false;
        }
    }

    QSqlQuery stamp(m_db);
    stamp.prepare(QStringLiteral("INSERT INTO schema_version (version, applied_at) VALUES (?, ?)"));
    stamp.addBindValue(SchemaVersion);
    stamp.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    if (!stamp.exec()) {
        if (error)
            *error = stamp.lastError().text();
        m_db.rollback();
        return false;
    }

    return m_db.commit();
}

} // namespace dollfin
