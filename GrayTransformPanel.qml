import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import test

ColumnLayout {
    id: root

    property alias closeButton: closeButton

    signal closeRequested()

    spacing: 0

    RowLayout {
        Layout.fillWidth: true
        Layout.preferredHeight: 44
        spacing: 0

        Label {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            text: qsTr("Gray Level Transform")
            font.pointSize: 12
            font.weight: Font.Medium
            verticalAlignment: Text.AlignVCenter
        }

        ToolButton {
            id: closeButton
            icon.source: "asserts/close.svg"
            icon.width: 16
            icon.height: 16
            onClicked: root.closeRequested()
            Accessible.name: qsTr("Close panel")
        }
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 1
        color: Material.color(Material.Grey, Material.Shade300)
    }

    Pane {
        Layout.fillWidth: true
        padding: 16
        topPadding: 12
        bottomPadding: 4
        Material.elevation: 0

        ColumnLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 4

            RowLayout {
                Layout.fillWidth: true

                Label {
                    text: qsTr("Min")
                    font.pointSize: 10
                }

                Item { Layout.fillWidth: true }

                Label {
                    text: Math.round(minSlider.value).toString()
                    font.pointSize: 10
                    color: Material.color(Material.Grey, Material.Shade600)
                }
            }

            Slider {
                id: minSlider
                Layout.fillWidth: true
                from: FitsManager.pixelMin
                to: FitsManager.pixelMax
                value: FitsManager.minValue
                stepSize: 1.0
                onMoved: FitsManager.minValue = Math.min(value, FitsManager.maxValue)
            }
        }
    }

    Pane {
        Layout.fillWidth: true
        padding: 16
        topPadding: 4
        bottomPadding: 12
        Material.elevation: 0

        ColumnLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 4

            RowLayout {
                Layout.fillWidth: true

                Label {
                    text: qsTr("Max")
                    font.pointSize: 10
                }

                Item { Layout.fillWidth: true }

                Label {
                    text: Math.round(maxSlider.value).toString()
                    font.pointSize: 10
                    color: Material.color(Material.Grey, Material.Shade600)
                }
            }

            Slider {
                id: maxSlider
                Layout.fillWidth: true
                from: FitsManager.pixelMin
                to: FitsManager.pixelMax
                value: FitsManager.maxValue
                stepSize: 1.0
                onMoved: FitsManager.maxValue = Math.max(value, FitsManager.minValue)
            }
        }
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 1
        Layout.leftMargin: 16
        Layout.rightMargin: 16
        color: Material.color(Material.Grey, Material.Shade200)
    }

    Pane {
        Layout.fillWidth: true
        padding: 16
        topPadding: 12
        Material.elevation: 0

        ColumnLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 8

            Button {
                Layout.fillWidth: true
                text: qsTr("Apply")
                enabled: FitsManager.status !== "loading"
                onClicked: FitsManager.applyGrayTransform()
                Material.background: Material.accent
                Material.foreground: "white"
            }

            Button {
                Layout.fillWidth: true
                text: qsTr("Auto Adjust")
                enabled: FitsManager.status !== "loading"
                flat: true
                onClicked: FitsManager.autoAdjust()
            }
        }
    }

    Item { Layout.fillHeight: true }
}
