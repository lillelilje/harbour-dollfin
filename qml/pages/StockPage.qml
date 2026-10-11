// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 lillelilje

import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    property int medicationId: -1
    property string medicationName: ""
    property var expiryDate: new Date()

    // [{key, label}]; the key is what gets stored (D-032).
    property var units: app.stockUnitOptions()

    allowedOrientations: Orientation.All

    function save() {
        app.saveStock(medicationId,
                      parseFloat(quantityField.text) || 0,
                      page.units[unitCombo.currentIndex].key,
                      parseFloat(lowField.text) || 0,
                      sourceField.text,
                      hasExpiry.checked ? Qt.formatDate(expiryDate, "yyyy-MM-dd") : "")
        pageStack.pop()
    }

    Component.onCompleted: {
        var m = app.medication(medicationId)
        medicationName = m.name
        quantityField.text = m.hasStock && m.quantity > 0 ? String(m.quantity) : ""
        lowField.text = m.lowThreshold > 0 ? String(m.lowThreshold) : ""
        sourceField.text = m.source
        var unitIndex = 0
        for (var i = 0; i < page.units.length; ++i) {
            if (page.units[i].key === m.stockUnit)
                unitIndex = i
        }
        unitCombo.currentIndex = unitIndex
        if (m.expires) {
            hasExpiry.checked = true
            expiryDate = new Date(m.expires + "T00:00:00")
        }
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        PullDownMenu {
            MenuItem { text: qsTr("Save"); onClicked: page.save() }
        }

        Column {
            id: column
            width: page.width
            spacing: Theme.paddingMedium

            PageHeader { title: qsTr("Stock") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: page.medicationName
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                truncationMode: TruncationMode.Fade
            }

            Row {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                spacing: Theme.paddingMedium

                TextField {
                    id: quantityField
                    width: parent.width * 0.5
                    label: qsTr("Quantity left")
                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                    validator: DoubleValidator {}
                }

                ComboBox {
                    id: unitCombo
                    width: parent.width * 0.5 - Theme.paddingMedium
                    label: qsTr("Unit")
                    menu: ContextMenu {
                        Repeater {
                            model: page.units
                            MenuItem { text: modelData.label }
                        }
                    }
                }
            }

            TextField {
                id: lowField
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                label: qsTr("Warn at or below")
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                validator: DoubleValidator {}
            }

            TextField {
                id: sourceField
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                label: qsTr("Source")
                placeholderText: qsTr("prescriber, pharmacy, …")
            }

            TextSwitch {
                id: hasExpiry
                text: qsTr("Has an expiry date")
            }

            ValueButton {
                visible: hasExpiry.checked
                label: qsTr("Expires")
                value: Qt.formatDate(page.expiryDate, "d MMM yyyy")
                onClicked: {
                    expiryDialog.date = page.expiryDate
                    expiryDialog.open()
                }
            }
        }
    }

    DatePickerDialog {
        id: expiryDialog
        onAccepted: page.expiryDate = selectedDate
    }
}
