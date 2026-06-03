import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

ColumnLayout {
    id: root

    required property var fitsManager
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
            icon.name: "close"
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
                from: root.fitsManager.pixelMin
                to: root.fitsManager.pixelMax
                value: root.fitsManager.minValue
                stepSize: 1.0
                onMoved: root.fitsManager.minValue = Math.min(value, root.fitsManager.maxValue)
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
                from: root.fitsManager.pixelMin
                to: root.fitsManager.pixelMax
                value: root.fitsManager.maxValue
                stepSize: 1.0
                onMoved: root.fitsManager.maxValue = Math.max(value, root.fitsManager.minValue)
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
                enabled: root.fitsManager.status !== "loading"
                onClicked: root.fitsManager.applyGrayTransform()
                Material.background: Material.accent
                Material.foreground: "white"
            }

            Button {
                Layout.fillWidth: true
                text: qsTr("Auto Adjust")
                enabled: root.fitsManager.status !== "loading"
                flat: true
                onClicked: root.fitsManager.autoAdjust()
            }
        }
    }

    Item { Layout.fillHeight: true }
}
