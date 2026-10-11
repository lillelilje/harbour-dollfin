// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 lillelilje

#pragma once

#include "core/database.h"
#include "core/store.h"

#include <QObject>
#include <QSettings>
#include <QVariantList>
#include <QVariantMap>

namespace dollfin {

// The single object exposed to QML as `app`. Owns the database and turns the
// core structs into plain QVariantMaps/lists. Lists are rebuilt and re-emitted
// on every change (the data set is tiny, so this stays simple and reactive).
class AppController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList medications READ medications NOTIFY changed)
    Q_PROPERTY(QVariantList regimens READ regimens NOTIFY changed)
    Q_PROPERTY(QVariantList todayDoses READ todayDoses NOTIFY changed)
    Q_PROPERTY(QVariantList history READ history NOTIFY changed)
    Q_PROPERTY(QVariantList labs READ labs NOTIFY changed)
    Q_PROPERTY(QVariantList analyteSummaries READ analyteSummaries NOTIFY changed)
    Q_PROPERTY(QString todayLabel READ todayLabel NOTIFY changed)
    Q_PROPERTY(bool remindersEnabled READ remindersEnabled NOTIFY settingsChanged)
    Q_PROPERTY(bool remindersConsent READ remindersConsent NOTIFY settingsChanged)

public:
    explicit AppController(const QString &settingsFile, QObject *parent = nullptr);

    bool initialize(const QString &databasePath, QString *error = nullptr);

    QVariantList medications() const;
    QVariantList regimens() const;
    QVariantList todayDoses() const;
    QVariantList history() const;
    QVariantList labs() const;
    QVariantList analyteSummaries() const;
    QString todayLabel() const;

    bool remindersEnabled() const;
    bool remindersConsent() const;

    Q_INVOKABLE QVariantMap medication(int id) const;
    Q_INVOKABLE QVariantMap regimen(int id) const;

    Q_INVOKABLE bool saveMedication(const QVariantMap &data);
    Q_INVOKABLE bool removeMedication(int id);
    Q_INVOKABLE bool saveRegimen(const QVariantMap &data);
    Q_INVOKABLE bool removeRegimen(int id);

    // status is "taken" or "skipped"; re-logging the same dose updates it.
    Q_INVOKABLE bool logDose(int regimenId, int medicationId,
                             const QString &scheduledAtIso, const QString &status);
    Q_INVOKABLE bool clearIntake(int intakeId);
    Q_INVOKABLE bool removeIntake(int intakeId);

    // Reminders (D-019/D-020).
    Q_INVOKABLE void setRemindersEnabled(bool enabled);
    Q_INVOKABLE void setRemindersConsent(bool consented);
    Q_INVOKABLE QString remindersIcs() const;
    Q_INVOKABLE bool syncCalendarReminders();
    Q_INVOKABLE QString exportIcs();
    Q_INVOKABLE bool openInCalendar(const QString &path);

    // Labs / blood results (P10, D-024).
    Q_INVOKABLE QVariantMap lab(int id) const;
    Q_INVOKABLE bool saveLab(const QVariantMap &data);
    Q_INVOKABLE bool removeLab(int id);
    Q_INVOKABLE QVariantList analyteCatalog() const;
    Q_INVOKABLE QVariantList labUnits() const;

    // Medication forms and stock units: [{key, label}], key is stored (D-032).
    Q_INVOKABLE QVariantList formOptions() const;
    Q_INVOKABLE QVariantList stockUnitOptions() const;

    // Stock (P5).
    Q_INVOKABLE bool saveStock(int medicationId, double quantity, const QString &unit,
                               double lowThreshold, const QString &source, const QString &expires);

    // Backup (P6).
    Q_INVOKABLE QString backupPath() const;
    Q_INVOKABLE QString exportBackup();
    Q_INVOKABLE bool importBackup(const QString &path);

signals:
    void changed();
    void settingsChanged();

private:
    QVariantMap medicationMap(const Medication &medication) const;
    void adjustStock(qint64 medicationId, double delta);

    Database m_database;
    Store m_store;
    QSettings m_settings;
};

} // namespace dollfin
