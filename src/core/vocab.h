// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 lillelilje

#pragma once

#include <QList>
#include <QSqlDatabase>
#include <QString>

namespace dollfin {

// A fixed choice stored as a stable English key and shown translated (D-032).
struct VocabEntry {
    QString key;
    QString label;
};

// Medication forms and stock units. The database stores the key; the label is
// for display only, so switching the UI language never breaks a lookup.
class Vocab
{
public:
    static QList<VocabEntry> forms();
    static QList<VocabEntry> stockUnits();

    static QString formLabel(const QString &key);
    static QString stockUnitLabel(const QString &key);

    // Map a value saved by an older version (the translated text shown when it
    // was picked) to its key. Keys and unknown text are returned unchanged.
    static QString normalizeForm(const QString &text);
    static QString normalizeStockUnit(const QString &text);

    // Rewrite medication.form and stock.unit to keys. Used by migration v3 and
    // after a backup import; runs inside the caller's transaction.
    static bool normalizeStored(QSqlDatabase &db, QString *error = nullptr);
};

} // namespace dollfin
