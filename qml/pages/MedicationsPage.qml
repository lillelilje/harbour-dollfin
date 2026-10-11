// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 lillelilje

import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    SilicaListView {
        id: list
        anchors.fill: parent
        model: app.medications
        header: PageHeader { title: qsTr("Medications") }

        PullDownMenu {
            MenuItem {
                text: qsTr("Add medication")
                onClicked: pageStack.push(Qt.resolvedUrl("MedicationEditPage.qml"))
            }
        }

        ViewPlaceholder {
            enabled: list.count === 0
            text: qsTr("No medications")
            hintText: qsTr("Pull down to add one")
        }

        delegate: ListItem {
            id: delegate
            contentHeight: Theme.itemSizeMedium

            menu: ContextMenu {
                MenuItem {
                    text: qsTr("Edit")
                    onClicked: pageStack.push(Qt.resolvedUrl("MedicationEditPage.qml"),
                                              { medicationId: modelData.id })
                }
                MenuItem {
                    text: qsTr("Stock")
                    onClicked: pageStack.push(Qt.resolvedUrl("StockPage.qml"),
                                              { medicationId: modelData.id })
                }
                MenuItem {
                    text: qsTr("Delete")
                    onClicked: Remorse.itemAction(delegate, qsTr("Deleting"),
                                               function() { app.removeMedication(modelData.id) })
                }
            }

            onClicked: pageStack.push(Qt.resolvedUrl("RegimensPage.qml"),
                                      { medicationId: modelData.id })

            Column {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter
                Label {
                    width: parent.width
                    text: modelData.name
                    color: delegate.highlighted ? Theme.highlightColor : Theme.primaryColor
                    truncationMode: TruncationMode.Fade
                }
                Label {
                    width: parent.width
                    visible: text.length > 0
                    text: modelData.detail
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeExtraSmall
                    truncationMode: TruncationMode.Fade
                }
                Label {
                    width: parent.width
                    visible: modelData.hasStock
                    text: modelData.quantity + " " + modelData.stockUnitLabel
                          + (modelData.isLow ? " · " + qsTr("low") : "")
                    color: modelData.isLow ? Theme.errorColor : Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeExtraSmall
                    truncationMode: TruncationMode.Fade
                }
            }
        }

        VerticalScrollDecorator {}
    }

}
