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
    property bool grayPanelOpen: false
    readonly property bool compactLayout: width < 760
    readonly property color statusError: "#F44336"
    readonly property color statusLoading: "#FF9800"
    readonly property color statusReady: "#4CAF50"

    Component.onCompleted: {
        if (fitsManager.errorMessage.length > 0)
            errorSnackbar.open()
    }

    Connections {
        target: fitsManager
        function onErrorMessageChanged() {
            if (fitsManager.errorMessage.length > 0)
                errorSnackbar.open()
        }
    }

    Connections {
        target: astrometryManager
        function onErrorMessageChanged() {
            if (astrometryManager.errorMessage.length > 0)
                astrometryProgress.open()
        }
    }

    Shortcut {
        sequence: "Ctrl+O"
        onActivated: fileDialog.open()
    }

    Shortcut {
        sequence: "+"
        enabled: root.hasImage
        onActivated: flickable.zoomCenter(1.15)
    }

    Shortcut {
        sequence: "="
        enabled: root.hasImage
        onActivated: flickable.zoomCenter(1.15)
    }

    Shortcut {
        sequence: "-"
        enabled: root.hasImage
        onActivated: flickable.zoomCenter(1 / 1.15)
    }

    Shortcut {
        sequence: "0"
        enabled: root.hasImage
        onActivated: flickable.resetView()
    }

    Shortcut {
        sequence: "Esc"
        onActivated: {
            if (root.grayPanelOpen)
                root.grayPanelOpen = false
            else if (sidebar.opened)
                sidebar.close()
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
                    icon.source: "asserts/close.svg"
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
                text: qsTr("Load FITS")
                onClicked: {
                    fileDialog.open()
                    sidebar.close()
                }
            }

            ItemDelegate {
                Layout.fillWidth: true
                text: qsTr("Manage Astrometry API")
                onClicked: {
                    apiDialog.open()
                    sidebar.close()
                }
            }

            ItemDelegate {
                Layout.fillWidth: true
                text: qsTr("Submit to Astrometry.net")
                enabled: fitsManager.tempFitsPath.length > 0 && !astrometryManager.running
                onClicked: {
                    astrometryProgress.open()
                    astrometryManager.submitFits(fitsManager.tempFitsPath)
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
                    visible: !root.hasImage && fitsManager.status !== "loading"
                }

                BusyIndicator {
                    anchors.centerIn: parent
                    running: fitsManager.status === "loading"
                    visible: running
                }

                Flickable {
                    id: flickable
                    anchors.fill: parent
                    anchors.margins: 16
                    visible: root.hasImage
                    clip: true
                    contentWidth: Math.max(width, fitsImage.width)
                    contentHeight: Math.max(height, fitsImage.height)
                    boundsBehavior: Flickable.StopAtBounds
                    interactive: root.hasImage
                    property bool followsFitScale: true
                    ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                    function resetView() {
                        followsFitScale = true
                        fitsImage.zoom = fitsImage.fitScale
                        contentX = 0
                        contentY = 0
                    }

                    function zoomAt(point, factor) {
                        let oldZoom = fitsImage.zoom
                        let newZoom = Math.max(0.05, Math.min(oldZoom * factor, 20.0))
                        if (Math.abs(newZoom - oldZoom) < 0.0001)
                            return

                        followsFitScale = false
                        let oldImageX = fitsImage.x
                        let oldImageY = fitsImage.y
                        let imageX = (contentX + point.x - oldImageX) / oldZoom
                        let imageY = (contentY + point.y - oldImageY) / oldZoom
                        fitsImage.zoom = newZoom
                        contentX = Math.max(0, Math.min(fitsImage.x + imageX * newZoom - point.x, contentWidth - width))
                        contentY = Math.max(0, Math.min(fitsImage.y + imageY * newZoom - point.y, contentHeight - height))
                    }

                    function zoomCenter(factor) {
                        zoomAt(Qt.point(width / 2, height / 2), factor)
                    }

                    Image {
                        id: fitsImage
                        source: fitsManager.imageSource
                        asynchronous: true
                        cache: false
                        fillMode: Image.PreserveAspectFit
                        readonly property real fitScale: Math.min(
                            flickable.width / Math.max(sourceSize.width, 1),
                            flickable.height / Math.max(sourceSize.height, 1),
                            1.0)
                        property real zoom: fitScale

                        width: sourceSize.width * zoom
                        height: sourceSize.height * zoom
                        x: Math.max(0, (flickable.contentWidth - width) / 2)
                        y: Math.max(0, (flickable.contentHeight - height) / 2)

                        onStatusChanged: {
                            if (status === Image.Ready)
                                flickable.resetView()
                        }

                        onFitScaleChanged: {
                            if (flickable.followsFitScale)
                                flickable.resetView()
                        }
                    }

                    WheelHandler {
                        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                        onWheel: function(event) {
                            let factor = event.angleDelta.y > 0 ? 1.15 : (1 / 1.15)
                            flickable.zoomAt(Qt.point(event.x, event.y), factor)
                        }
                    }
                }

                // Double-click to reset zoom
                TapHandler {
                    enabled: root.hasImage
                    onDoubleTapped: {
                        flickable.resetView()
                    }
                }
            }

            // -- Right activity bar --
            Pane {
                Layout.fillHeight: true
                Layout.preferredWidth: 40
                visible: root.hasImage
                padding: 4
                Material.elevation: 0

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 4

                    ToolButton {
                        Layout.alignment: Qt.AlignHCenter
                        icon.source: "asserts/contrast.svg"
                        icon.width: 22
                        icon.height: 22
                        checked: root.grayPanelOpen
                        checkable: true
                        onToggled: root.grayPanelOpen = checked
                        Accessible.name: qsTr("Gray level transform")

                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Gray Level Transform")
                        ToolTip.delay: 500
                    }

                    Item { Layout.fillHeight: true }
                }
            }

            // -- Gray transform right panel --
            Pane {
                Layout.fillHeight: true
                Layout.preferredWidth: 280
                visible: root.grayPanelOpen && !root.compactLayout
                padding: 0
                Material.elevation: 1

                GrayTransformPanel {
                    anchors.fill: parent
                    fitsManager: fitsManager
                    onCloseRequested: root.grayPanelOpen = false
                }
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

                RowLayout {
                    Layout.rightMargin: 16
                    spacing: 6

                    Rectangle {
                        Layout.preferredWidth: 8
                        Layout.preferredHeight: 8
                        radius: 4
                        color: astrometryManager.apiKeyConfigured ? root.statusReady : root.statusError
                        Accessible.ignored: true
                    }

                    Label {
                        text: astrometryManager.apiKeyConfigured ? qsTr("API configured") : qsTr("API missing")
                        font.pointSize: 10
                        color: Material.color(Material.Grey, Material.Shade700)
                    }
                }

                RowLayout {
                    Layout.rightMargin: 16
                    spacing: 6
                    visible: astrometryManager.running || astrometryManager.status === "success" || astrometryManager.status === "failed"

                    BusyIndicator {
                        Layout.preferredWidth: 16
                        Layout.preferredHeight: 16
                        running: astrometryManager.running
                        visible: running
                    }

                    Label {
                        text: astrometryManager.running
                            ? qsTr("Astrometry %1%").arg(Math.round(astrometryManager.progress * 100))
                            : astrometryManager.status === "success" ? qsTr("Astrometry done")
                            : astrometryManager.status === "failed" ? qsTr("Astrometry failed")
                            : ""
                        font.pointSize: 10
                        color: Material.color(Material.Grey, Material.Shade700)
                    }
                }

                Rectangle {
                    Layout.preferredWidth: 8
                    Layout.preferredHeight: 8
                    radius: 4
                    color: fitsManager.status === "error" ? root.statusError
                         : fitsManager.status === "loading" ? root.statusLoading
                         : root.statusReady

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

    Dialog {
        id: apiDialog
        title: qsTr("Astrometry.net API")
        modal: true
        standardButtons: Dialog.NoButton
        width: Math.min(root.width - 48, 440)
        x: (root.width - width) / 2
        y: Math.max(48, (root.height - height) / 2)

        ColumnLayout {
            anchors.fill: parent
            spacing: 12

            Label {
                Layout.fillWidth: true
                text: astrometryManager.apiKeyConfigured
                    ? qsTr("API key configured: %1").arg(astrometryManager.apiKeyPreview())
                    : qsTr("API key is not configured.")
                wrapMode: Text.WordWrap
            }

            TextField {
                id: apiKeyField
                Layout.fillWidth: true
                echoMode: TextInput.Password
                placeholderText: qsTr("Astrometry.net API key")
            }

            RowLayout {
                Layout.fillWidth: true

                Button {
                    text: qsTr("Save")
                    enabled: apiKeyField.text.trim().length > 0
                    onClicked: {
                        astrometryManager.saveApiKey(apiKeyField.text)
                        apiKeyField.clear()
                        apiDialog.close()
                    }
                }

                Button {
                    text: qsTr("Clear")
                    flat: true
                    enabled: astrometryManager.apiKeyConfigured
                    onClicked: {
                        astrometryManager.clearApiKey()
                        apiKeyField.clear()
                    }
                }

                Item { Layout.fillWidth: true }

                Button {
                    text: qsTr("Close")
                    flat: true
                    onClicked: apiDialog.close()
                }
            }
        }
    }

    Dialog {
        id: astrometryProgress
        title: qsTr("Astrometry.net Solve")
        modal: false
        standardButtons: Dialog.NoButton
        width: Math.min(root.width - 48, 520)
        x: (root.width - width) / 2
        y: Math.max(48, root.height - height - 72)

        function openForCurrentTask() { open() }

        ColumnLayout {
            anchors.fill: parent
            spacing: 10

            Label {
                Layout.fillWidth: true
                text: astrometryManager.stageText
                font.weight: Font.Medium
                wrapMode: Text.WordWrap
            }

            ProgressBar {
                Layout.fillWidth: true
                from: 0
                to: 1
                indeterminate: astrometryManager.running && astrometryManager.progress < 0.05
                value: astrometryManager.progress
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("Elapsed: %1s%2%3")
                    .arg(astrometryManager.elapsedSeconds)
                    .arg(astrometryManager.subId.length > 0 ? qsTr("  SubID: %1").arg(astrometryManager.subId) : "")
                    .arg(astrometryManager.jobId.length > 0 ? qsTr("  JobID: %1").arg(astrometryManager.jobId) : "")
                font.pointSize: 10
                color: Material.color(Material.Grey, Material.Shade700)
                wrapMode: Text.WordWrap
            }

            Label {
                Layout.fillWidth: true
                text: astrometryManager.outputDir.length > 0 ? qsTr("Saved to: %1").arg(astrometryManager.outputDir) : ""
                visible: text.length > 0
                font.pointSize: 10
                color: Material.color(Material.Grey, Material.Shade700)
                wrapMode: Text.WordWrap
            }

            Label {
                Layout.fillWidth: true
                text: astrometryManager.errorMessage
                visible: text.length > 0
                color: root.statusError
                wrapMode: Text.WordWrap
            }

            RowLayout {
                Layout.fillWidth: true

                Button {
                    text: qsTr("Cancel")
                    visible: astrometryManager.running
                    onClicked: astrometryManager.cancel()
                }

                Item { Layout.fillWidth: true }

                Button {
                    text: qsTr("Close")
                    flat: true
                    enabled: !astrometryManager.running
                    onClicked: astrometryProgress.close()
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

    Drawer {
        id: compactGrayPanel
        width: Math.min(root.width * 0.85, 320)
        height: root.height
        edge: Qt.RightEdge
        modal: true
        dim: true
        visible: root.compactLayout && root.grayPanelOpen
        onClosed: root.grayPanelOpen = false

        GrayTransformPanel {
            anchors.fill: parent
            fitsManager: fitsManager
            onCloseRequested: root.grayPanelOpen = false
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
                            color: root.statusError
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
