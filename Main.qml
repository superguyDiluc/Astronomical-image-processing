import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    id: root

    width: 960
    height: 680
    minimumWidth: 480
    minimumHeight: 360
    visible: true
    title: qsTr("FITS Viewer")

    Material.theme: Material.Light
    Material.accent: Material.Indigo

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            spacing: 4

            ToolButton {
                icon.source: "asserts/list.svg"
                icon.width: 20
                icon.height: 20
                onClicked: sidebar.open()
                Accessible.name: qsTr("Open sidebar")
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("FITS Viewer")
                font.pointSize: 12
                font.weight: Font.Medium
                elide: Text.ElideRight
            }
        }
    }

    Drawer {
        id: sidebar
        width: Math.min(root.width * 0.7, 280)
        height: root.height
        edge: Qt.LeftEdge
        modal: true
        dim: true

        topPadding: 0
        bottomPadding: 0
        leftPadding: 0
        rightPadding: 0

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 48
                spacing: 0

                Label {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    text: qsTr("Tools")
                    font.pointSize: 14
                    font.weight: Font.Medium
                    verticalAlignment: Text.AlignVCenter
                }

                ToolButton {
                    icon.name: "close"
                    icon.width: 18
                    icon.height: 18
                    onClicked: sidebar.close()
                    Accessible.name: qsTr("Close sidebar")
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: Material.color(Material.Grey, Material.Shade300)
            }

            ItemDelegate {
                Layout.fillWidth: true
                icon.name: "folder-open"
                text: qsTr("Load FITS")
                onClicked: {
                    fileDialog.open()
                    sidebar.close()
                }
            }

            Item { Layout.fillHeight: true }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Label {
                anchors.centerIn: parent
                text: qsTr("Open a FITS file to begin")
                font.pointSize: 14
                color: Material.color(Material.Grey)
            }
        }

        Pane {
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            padding: 0
            topPadding: 0
            bottomPadding: 0
            leftPadding: 8
            rightPadding: 8

            Material.elevation: 0

            RowLayout {
                anchors.fill: parent
                spacing: 0

                Item { Layout.fillWidth: true }

                Rectangle {
                    Layout.preferredWidth: 8
                    Layout.preferredHeight: 8
                    radius: 4
                    color: "#4CAF50"

                    Accessible.ignored: true
                }

                Label {
                    Layout.leftMargin: 6
                    text: qsTr("Ready")
                    font.pointSize: 10
                    color: Material.color(Material.Grey, Material.Shade700)
                }
            }
        }
    }

    Loader {
        id: fileDialog
        active: false
        function open() { active = true }

        sourceComponent: Component {
            FileDialog {
                id: dialog
                title: qsTr("Select FITS file")
                nameFilters: [qsTr("FITS files (*.fits *.fit *.fts)")]
                onAccepted: fileDialog.active = false
                onRejected: fileDialog.active = false
                Component.onCompleted: open()
            }
        }
    }
}
