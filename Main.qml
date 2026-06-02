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
                icon.name: "folder-open"
                text: qsTr("Load FITS")
                onClicked: fileDialog.open()
            }

            Item { Layout.fillWidth: true }
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
            Layout.preferredHeight: 28
            padding: 4

            Material.elevation: 0

            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Ready")
                font.pointSize: 10
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
