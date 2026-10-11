// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 lillelilje

#include "vocab.h"

#include <QCoreApplication>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace dollfin {
namespace {

// QT_TRANSLATE_NOOP marks the literal for lupdate; the key doubles as the
// English label and the translation happens in toEntries().
const char *const kForms[] = {
    QT_TRANSLATE_NOOP("Dollfin", "Pill"),
    QT_TRANSLATE_NOOP("Dollfin", "Gel"),
    QT_TRANSLATE_NOOP("Dollfin", "Patch"),
    QT_TRANSLATE_NOOP("Dollfin", "Injection"),
    QT_TRANSLATE_NOOP("Dollfin", "Spray"),
    QT_TRANSLATE_NOOP("Dollfin", "Implant"),
    QT_TRANSLATE_NOOP("Dollfin", "Other"),
};

const char *const kStockUnits[] = {
    QT_TRANSLATE_NOOP("Dollfin", "doses"),
    QT_TRANSLATE_NOOP("Dollfin", "tablets"),
    QT_TRANSLATE_NOOP("Dollfin", "capsules"),
    QT_TRANSLATE_NOOP("Dollfin", "patches"),
    QT_TRANSLATE_NOOP("Dollfin", "sachets"),
    QT_TRANSLATE_NOOP("Dollfin", "vials"),
    QT_TRANSLATE_NOOP("Dollfin", "ml"),
    QT_TRANSLATE_NOOP("Dollfin", "g"),
};

// Translated text that versions before D-032 stored instead of the key.
// Frozen: taken from the nl/nb translations of 1.2; never extend it, new
// languages only ever store keys.
struct Legacy {
    const char *text;
    const char *key;
};

const Legacy kLegacyForms[] = {
    {"Pil", "Pill"}, {"Pleister", "Patch"}, {"Injectie", "Injection"},
    {"Implantaat", "Implant"}, {"Anders", "Other"},
    {"Pille", "Pill"}, {"Plaster", "Patch"}, {"Injeksjon", "Injection"},
    {"Implantat", "Implant"}, {"Annet", "Other"},
};

const Legacy kLegacyStockUnits[] = {
    {"tabletten", "tablets"}, {"pleisters", "patches"}, {"flesjes", "vials"},
    {"doser", "doses"}, {"tabletter", "tablets"}, {"plaster", "patches"}, {"poser", "sachets"},
    {"hetteglass", "vials"},
};

template <int N>
QList<VocabEntry> toEntries(const char *const (&keys)[N])
{
    QList<VocabEntry> list;
    for (const char *key : keys)
        list.append({QString::fromLatin1(key), QCoreApplication::translate("Dollfin", key)});
    return list;
}

template <int N>
QString normalize(const Legacy (&table)[N], const QString &text)
{
    for (const Legacy &legacy : table) {
        if (text == QLatin1String(legacy.text))
            return QString::fromLatin1(legacy.key);
    }
    return text;
}

template <int N>
bool rewrite(QSqlDatabase &db, const Legacy (&table)[N], const char *sql, QString *error)
{
    for (const Legacy &legacy : table) {
        QSqlQuery query(db);
        query.prepare(QString::fromLatin1(sql));
        query.addBindValue(QString::fromLatin1(legacy.key));
        query.addBindValue(QString::fromLatin1(legacy.text));
        if (!query.exec()) {
            if (error)
                *error = query.lastError().text();
            return false;
        }
    }
    return true;
}

QString labelFor(const QList<VocabEntry> &entries, const QString &key)
{
    for (const VocabEntry &entry : entries) {
        if (entry.key == key)
            return entry.label;
    }
    return key; // free text from an unknown source: show as stored
}

} // namespace

QList<VocabEntry> Vocab::forms()
{
    return toEntries(kForms);
}

QList<VocabEntry> Vocab::stockUnits()
{
    return toEntries(kStockUnits);
}

QString Vocab::formLabel(const QString &key)
{
    return labelFor(forms(), key);
}

QString Vocab::stockUnitLabel(const QString &key)
{
    return labelFor(stockUnits(), key);
}

QString Vocab::normalizeForm(const QString &text)
{
    return normalize(kLegacyForms, text);
}

QString Vocab::normalizeStockUnit(const QString &text)
{
    return normalize(kLegacyStockUnits, text);
}

bool Vocab::normalizeStored(QSqlDatabase &db, QString *error)
{
    return rewrite(db, kLegacyForms, "UPDATE medication SET form=? WHERE form=?", error)
        && rewrite(db, kLegacyStockUnits, "UPDATE stock SET unit=? WHERE unit=?", error);
}

} // namespace dollfin
