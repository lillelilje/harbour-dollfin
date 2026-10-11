// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 lillelilje

#pragma once

#include <QDate>
#include <QDateTime>
#include <QList>
#include <QString>
#include <QTime>

namespace dollfin {

struct Medication {
    qint64 id = -1;
    QString name;          // free text, no fixed catalogue (D-017)
    QString form;          // tablet, gel, patch, injection, ...
    double strength = 0.0; // 0 = unspecified
    QString unit;          // mg, ug, ml, ...
    QString notes;
    bool active = true;
};

struct Regimen {
    qint64 id = -1;
    qint64 medicationId = -1;
    double dose = 0.0;     // units per dose (pills, patches, sprays); 0 = unspecified (D-031)
    QString unit;
    QString route;
    int intervalDays = 1;  // used when daysOfWeek is empty
    QList<int> daysOfWeek; // 1=Mon .. 7=Sun; empty => every intervalDays days
    QList<QTime> times;    // local times of day
    QDate startDate;
    QDate endDate;         // invalid => open-ended
    bool active = true;
};

// Countable units taken per dose; an unspecified count means one (D-031).
inline double unitsPerDose(const Regimen &regimen)
{
    return regimen.dose > 0.0 ? regimen.dose : 1.0;
}

struct Intake {
    qint64 id = -1;
    qint64 regimenId = -1;
    qint64 medicationId = -1;
    QDateTime scheduledAt; // invalid => ad-hoc
    QDateTime takenAt;
    double amount = 0.0;
    QString unit;
    QString status;        // "taken" | "skipped"
    QString site;          // application site (backlog, D-015)
    QString notes;
    double stockDelta = -1.0; // taken off stock when logged; -1 = unknown (before D-033)
};

struct Stock {
    qint64 medicationId = -1;
    double quantity = 0.0;
    QString unit;
    double lowThreshold = 0.0;
    QDate lastRefill;
    QDate expires;
    QString source; // prescriber, pharmacy, ...
};

struct Level {
    qint64 id = -1;
    QDateTime takenAt;
    QString analyte;
    double value = 0.0;
    QString unit;
    QString lab;
    QString notes;
};

} // namespace dollfin
