// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 lillelilje

import QtQuick 2.0
import Sailfish.Silica 1.0

// Shown after a medication's form changed while it has schedules (D-031):
// the units per dose on those schedules may no longer fit. Replaces the
// medication page, so Later goes back to the list and Now replaces this
// dialog with the schedule(s).
Dialog {
    id: dialog

    property int medicationId: -1
    property int regimenId: -1 // set when there is exactly one schedule

    acceptDestination: regimenId >= 0 ? Qt.resolvedUrl("RegimenEditPage.qml")
                                      : Qt.resolvedUrl("RegimensPage.qml")
    acceptDestinationProperties: regimenId >= 0
                                 ? { "medicationId": medicationId, "regimenId": regimenId }
                                 : { "medicationId": medicationId }
    acceptDestinationAction: PageStackAction.Replace

    Column {
        width: parent.width
        spacing: Theme.paddingMedium

        DialogHeader {
            dialog: dialog
            title: qsTr("Update schedules?")
            acceptText: qsTr("Now")
            cancelText: qsTr("Later")
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * Theme.horizontalPageMargin
            wrapMode: Text.WordWrap
            font.pixelSize: Theme.fontSizeSmall
            text: qsTr("The form changed. Check this medication's schedules, for example the amount per dose.")
        }
    }
}
