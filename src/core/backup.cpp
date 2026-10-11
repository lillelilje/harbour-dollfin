// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 lillelilje

#include "backup.h"

#include "database.h"
#include "vocab.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStringList>
#include <QVariant>

namespace dollfin {
namespace {

QStringList allTables()
{
    return QStringList() << QStringLiteral("medication") << QStringLiteral("regimen")
                         << QStringLiteral("intake") << QStringLiteral("stock")
                         << QStringLiteral("level");
}

} // namespace

QString Backup::toJson(const Database &database)
{
    QJsonObject root;
    root[QStringLiteral("format")] = QStringLiteral("dollfin-backup");
    root[QStringLiteral("version")] = 1;
    root[QStringLiteral("schemaVersion")] = Database::SchemaVersion;
    root[QStringLiteral("exportedAt")] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    QJsonObject tables;
    const QStringList names = allTables();
    for (const QString &name : names) {
        QJsonArray rows;
        QSqlQuery query(database.handle());
        if (query.exec(QStringLiteral("SELECT * FROM ") + name)) {
            while (query.next()) {
                const QSqlRecord record = query.record();
                QJsonObject row;
                for (int i = 0; i < record.count(); ++i)
                    row[record.fieldName(i)] = QJsonValue::fromVariant(query.value(i));
                rows.append(row);
            }
        }
        tables[name] = rows;
    }
    root[QStringLiteral("tables")] = tables;
    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

bool Backup::fromJson(Database &database, const QString &json, QString *error)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(json.toUtf8(), &parseError);
    if (document.isNull()) {
        if (error)
            *error = parseError.errorString();
        return false;
    }

    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("format")).toString() != QStringLiteral("dollfin-backup")) {
        if (error)
            *error = QStringLiteral("Not a Dollfin backup");
        return false;
    }
    const QJsonObject tables = root.value(QStringLiteral("tables")).toObject();

    QSqlDatabase db = database.handle();
    if (!db.transaction()) {
        if (error)
            *error = db.lastError().text();
        return false;
    }

    // Children first on delete, parents first on insert (foreign keys).
    const QStringList deleteOrder = QStringList() << QStringLiteral("intake")
                                                  << QStringLiteral("stock")
                                                  << QStringLiteral("level")
                                                  << QStringLiteral("regimen")
                                                  << QStringLiteral("medication");
    const QStringList insertOrder = QStringList() << QStringLiteral("medication")
                                                  << QStringLiteral("regimen")
                                                  << QStringLiteral("intake")
                                                  << QStringLiteral("stock")
                                                  << QStringLiteral("level");

    for (const QString &name : deleteOrder) {
        QSqlQuery query(db);
        if (!query.exec(QStringLiteral("DELETE FROM ") + name)) {
            if (error)
                *error = query.lastError().text();
            db.rollback();
            return false;
        }
    }

    for (const QString &name : insertOrder) {
        const QJsonArray rows = tables.value(name).toArray();
        for (const QJsonValue &value : rows) {
            const QJsonObject row = value.toObject();
            if (row.isEmpty())
                continue;

            QStringList columns;
            QVariantList values;
            for (auto it = row.constBegin(); it != row.constEnd(); ++it) {
                columns.append(it.key());
                values.append(it.value().toVariant());
            }

            QStringList placeholders;
            for (int i = 0; i < columns.size(); ++i)
                placeholders.append(QStringLiteral("?"));

            QSqlQuery query(db);
            query.prepare(QStringLiteral("INSERT INTO %1 (%2) VALUES (%3)")
                              .arg(name, columns.join(QStringLiteral(", ")),
                                   placeholders.join(QStringLiteral(", "))));
            for (const QVariant &v : values)
                query.addBindValue(v);
            if (!query.exec()) {
                if (error)
                    *error = query.lastError().text();
                db.rollback();
                return false;
            }
        }
    }

    // Backups from before D-032 hold translated forms/stock units.
    if (!Vocab::normalizeStored(db, error)) {
        db.rollback();
        return false;
    }

    if (!db.commit()) {
        if (error)
            *error = db.lastError().text();
        return false;
    }
    return true;
}

} // namespace dollfin
