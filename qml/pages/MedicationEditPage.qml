// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 lillelilje

import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    property int medicationId: -1
    property string originalForm: ""

    // Index 0 is always "Custom": pick it to type any medication name (D-017).
    property var medicationNames: [
        qsTr("Custom"),
        qsTr("Estradiol"),
        qsTr("Estradiol valerate"),
        qsTr("Estradiol hemihydrate"),
        qsTr("Estradiol cypionate"),
        qsTr("Estradiol enanthate"),
        qsTr("Estradiol benzoate"),
        qsTr("Estradiol undecylate"),
        qsTr("Conjugated estrogens"),
        qsTr("Ethinylestradiol"),
        qsTr("Testosterone"),
        qsTr("Testosterone enanthate"),
        qsTr("Testosterone cypionate"),
        qsTr("Testosterone undecanoate"),
        qsTr("Spironolactone"),
        qsTr("Cyproterone acetate"),
        qsTr("Bicalutamide"),
        qsTr("Finasteride"),
        qsTr("Dutasteride"),
        qsTr("Leuprorelin"),
        qsTr("Goserelin"),
        qsTr("Triptorelin"),
        qsTr("Progesterone"),
        qsTr("Medroxyprogesterone acetate")
    ]

    // [{key, label}]; the key is what gets stored (D-032). Last entry is "Other".
    property var methods: app.formOptions()

    // Micro sign (U+00B5) as used on packaging.
    property var units: ["mg", "µg", "g", "ml", "IU", "%"]

    allowedOrientations: Orientation.All

    function save() {
        var name = medCombo.currentIndex === 0 ? nameField.text
                                               : page.medicationNames[medCombo.currentIndex]
        var data = {
            "id": medicationId,
            "name": name,
            "form": page.methods[methodCombo.currentIndex].key,
            "strength": parseFloat(strengthField.text) || 0,
            "unit": page.units[unitCombo.currentIndex],
            "notes": notesField.text,
            "active": true
        }
        if (app.saveMedication(data)) {
            var schedules = medicationId >= 0 && data.form !== originalForm
                            ? schedulesForMedication() : []
            if (schedules.length > 0) {
                // The units per dose may no longer fit the new form (D-031).
                pageStack.replace(Qt.resolvedUrl("ScheduleUpdateDialog.qml"), {
                    "medicationId": medicationId,
                    "regimenId": schedules.length === 1 ? schedules[0].id : -1
                })
            } else {
                pageStack.pop()
            }
        } else {
            nameField.errorHighlight = true
            nameField.placeholderText = qsTr("A name is required")
        }
    }

    function schedulesForMedication() {
        var all = app.regimens
        var out = []
        for (var i = 0; i < all.length; ++i) {
            if (all[i].medicationId === medicationId)
                out.push(all[i])
        }
        return out
    }

    Component.onCompleted: {
        methodCombo.currentIndex = page.methods.length - 1
        unitCombo.currentIndex = 0
        if (medicationId >= 0) {
            var m = app.medication(medicationId)
            var nameIndex = page.medicationNames.indexOf(m.name)
            if (nameIndex > 0) {
                medCombo.currentIndex = nameIndex
            } else {
                medCombo.currentIndex = 0
                nameField.text = m.name
            }
            originalForm = m.form
            var methodIndex = -1
            for (var i = 0; i < page.methods.length; ++i) {
                if (page.methods[i].key === m.form)
                    methodIndex = i
            }
            methodCombo.currentIndex = methodIndex >= 0 ? methodIndex : page.methods.length - 1
            strengthField.text = m.strength > 0 ? String(m.strength) : ""
            var unitIndex = page.units.indexOf(m.unit)
            unitCombo.currentIndex = unitIndex >= 0 ? unitIndex : 0
            notesField.text = m.notes
        } else {
            medCombo.currentIndex = 0
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

            PageHeader {
                title: medicationId >= 0 ? qsTr("Edit medication") : qsTr("New medication")
            }

            ComboBox {
                id: medCombo
                label: qsTr("Medication")
                menu: ContextMenu {
                    Repeater {
                        model: page.medicationNames
                        MenuItem { text: modelData }
                    }
                }
            }

            TextField {
                id: nameField
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: medCombo.currentIndex === 0
                label: qsTr("Custom name")
                placeholderText: qsTr("Any medication you take")
                EnterKey.enabled: text.length > 0
                EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                EnterKey.onClicked: page.save()
            }

            ComboBox {
                id: methodCombo
                label: qsTr("Method")
                menu: ContextMenu {
                    Repeater {
                        model: page.methods
                        MenuItem { text: modelData.label }
                    }
                }
            }

            Row {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                spacing: Theme.paddingMedium

                TextField {
                    id: strengthField
                    width: parent.width * 0.5
                    label: qsTr("Strength")
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
                            MenuItem { text: modelData }
                        }
                    }
                }
            }

            TextArea {
                id: notesField
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                label: qsTr("Notes")
                placeholderText: qsTr("Optional")
            }
        }
    }
}
