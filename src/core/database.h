// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 lillelilje

#pragma once

#include <QSqlDatabase>
#include <QString>

namespace dollfin {

// Owns the SQLite connection and applies versioned migrations.
// Timestamps are stored as UTC unix seconds; dates as "yyyy-MM-dd".
class Database
{
public:
    static const int SchemaVersion = 4;

    Database();
    ~Database();

    // Opens (creating it if needed) the database at `filePath` and migrates it.
    bool open(const QString &filePath, QString *error = nullptr);
    void close();
    bool isOpen() const;
    QSqlDatabase handle() const { return m_db; }
    QString filePath() const { return m_filePath; }

    // Highest applied migration, or 0 for an empty database.
    int schemaVersion() const;

private:
    bool migrate(QString *error);
    bool run(const QString &sql, QString *error);
    bool hasColumn(const QString &table, const QString &column) const;

    QString m_filePath;
    QString m_connectionName;
    QSqlDatabase m_db;
};

} // namespace dollfin
