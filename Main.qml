import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Dialogs
import test

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

    readonly property bool hasImage: fitsManager.imageSource.length > 0

    FitsManager {
        id: fitsManager
        onErrorMessageChanged: {
            if (errorMessage.length > 0)
                errorSnackbar.open()
        }
    }

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

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // -- Image viewport with zoom & pan --
            Item {
                id: viewportContainer
                Layout.fillWidth: true
                Layout.fillHeight: true

                Label {
                    anchors.centerIn: parent
                    text: qsTr("Open a FITS file to begin")
                    font.pointSize: 14
                    color: Material.color(Material.Grey)
                    visible: !root.hasImage
                }

                Flickable {
                    id: flickable
                    anchors.fill: parent
                    anchors.margins: 16
                    visible: root.hasImage
                    clip: true
                    contentWidth: fitsImage.width * fitsImage.scale
                    contentHeight: fitsImage.height * fitsImage.scale
                    boundsBehavior: Flickable.StopAtBounds

                    Image {
                        id: fitsImage
                        source: fitsManager.imageSource
                        asynchronous: true
                        cache: false
                        fillMode: Image.PreserveAspectFit
                        transformOrigin: Item.TopLeft

                        readonly property real fitScale: Math.min(
                            flickable.width / Math.max(sourceSize.width, 1),
                            flickable.height / Math.max(sourceSize.height, 1),
                            1.0)

                        width: sourceSize.width
                        height: sourceSize.height
                        scale: fitScale

                        onStatusChanged: {
                            if (status === Image.Ready)
                                scale = fitScale
                        }
                    }

                    WheelHandler {
                        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                        onWheel: function(event) {
                            let factor = event.angleDelta.y > 0 ? 1.15 : (1 / 1.15)
                            let newScale = Math.max(0.05,
                                Math.min(fitsImage.scale * factor, 20.0))
                            fitsImage.scale = newScale
                        }
                    }

                    BusyIndicator {
                        anchors.centerIn: parent
                        running: fitsManager.status === "loading"
                        visible: running
                    }
                }

                // Double-click to reset zoom
                TapHandler {
                    enabled: root.hasImage
                    onDoubleTapped: {
                        fitsImage.scale = fitsImage.fitScale
                        flickable.contentX = 0
                        flickable.contentY = 0
                    }
                }
            }

            // -- Right activity bar (placeholder, Task 2 fills this) --

            // -- Right panel (placeholder, Task 4 fills this) --
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
                    color: fitsManager.status === "error" ? "#F44336"
                         : fitsManager.status === "loading" ? "#FF9800"
                         : "#4CAF50"

                    Accessible.ignored: true
                }

                Label {
                    Layout.leftMargin: 6
                    text: fitsManager.status === "error" ? qsTr("Error")
                        : fitsManager.status === "loading" ? qsTr("Loading...")
                        : qsTr("Ready")
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
                onAccepted: {
                    fitsManager.loadFile(selectedFile)
                    fileDialog.active = false
                }
                onRejected: fileDialog.active = false
                Component.onCompleted: open()
            }
        }
    }

    Loader {
        id: errorSnackbar
        active: false
        function open() { active = true }

        sourceComponent: Component {
            Popup {
                id: snackbar
                parent: Overlay.overlay
                x: (parent.width - width) / 2
                y: parent.height - height - 48
                width: Math.min(parent.width - 32, 480)
                modal: false
                closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
                padding: 0

                onClosed: {
                    fitsManager.clearError()
                    errorSnackbar.active = false
                }
                Component.onCompleted: open()

                Timer {
                    interval: 6000
                    running: snackbar.visible
                    onTriggered: snackbar.close()
                }

                Pane {
                    anchors.fill: parent
                    Material.elevation: 6

                    RowLayout {
                        anchors.fill: parent
                        spacing: 8

                        Label {
                            Layout.fillWidth: true
                            text: fitsManager.errorMessage
                            font.pointSize: 10
                            color: "#F44336"
                            wrapMode: Text.WordWrap
                            maximumLineCount: 3
                            elide: Text.ElideRight
                        }

                        ToolButton {
                            text: qsTr("Dismiss")
                            flat: true
                            onClicked: snackbar.close()
                            Material.foreground: Material.accent
                        }
                    }
                }
            }
        }
    }
}
