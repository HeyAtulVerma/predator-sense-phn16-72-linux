import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ApplicationWindow {
    id: win
    width: 1240
    height: 820
    minimumWidth: 1040
    minimumHeight: 700
    visible: !startHidden
    color: Theme.bg
    title: "Predator Sense"
    font.family: Theme.font

    property int page: 0
    readonly property var info: backend.info
    readonly property var st: backend.state
    readonly property var sn: backend.sensors

    function temp(c) {
        if (c === undefined || c === null) return "—"
        return backend.fahrenheit ? Math.round(c * 9 / 5 + 32) : Math.round(c)
    }
    readonly property string tempUnit: backend.fahrenheit ? "°F" : "°C"

    onClosing: (close) => {
        if (backend.closeToTray && trayAvailable) {
            close.accepted = false
            win.hide()
        }
    }
    onVisibleChanged: backend.monitoring = visible
    Component.onCompleted: backend.monitoring = visible

    // Sensor history for the charts (kept while the window is open)
    QtObject {
        id: history
        property int capacity: 120
        property var cpuTemp: []
        property var gpuTemp: []
        property var cpuFan: []
        property var gpuFan: []
        property var cpuUsage: []
        property var gpuUsage: []
        property int revision: 0
        function push(arr, v) { arr.push(v === undefined ? NaN : v); if (arr.length > capacity) arr.shift() }
    }
    Connections {
        target: backend
        function onSensorsChanged() {
            const s = backend.sensors
            history.push(history.cpuTemp, s.cpuTemp)
            history.push(history.gpuTemp, s.gpu1Temp)
            history.push(history.cpuFan, s.cpuFan)
            history.push(history.gpuFan, s.gpu1Fan)
            history.push(history.cpuUsage, s.cpuUsage)
            history.push(history.gpuUsage, s.gpuIdle ? 0 : s.gpuUsage)
            history.revision++
        }
        function onErrorOccurred(message) { toast.show(message) }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ---------------------------------------------------------- sidebar
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 230
            color: Theme.bgDeep
            ColumnLayout {
                anchors.fill: parent
                spacing: 0
                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 96
                    Row {
                        anchors.centerIn: parent
                        spacing: 10
                        Image { source: "qrc:/icons/predator-sense.svg"; width: 34; height: 34; sourceSize: Qt.size(68, 68); anchors.verticalCenter: parent.verticalCenter }
                        Column {
                            anchors.verticalCenter: parent.verticalCenter
                            Text { text: "PREDATOR"; color: Theme.text; font.pixelSize: 17; font.weight: Font.Bold; font.letterSpacing: 3 }
                            Text { text: "SENSE  ·  LINUX"; color: Theme.accent; font.pixelSize: 10; font.letterSpacing: 2.5 }
                        }
                    }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.line }
                Item { Layout.preferredHeight: 10 }
                NavButton { label: qsTr("Início"); glyph: "◈"; active: win.page === 0; onClicked: win.page = 0 }
                NavButton { label: qsTr("Ventoinhas"); glyph: "✣"; active: win.page === 1; onClicked: win.page = 1 }
                NavButton { label: qsTr("Iluminação"); glyph: "✦"; active: win.page === 2; onClicked: win.page = 2 }
                NavButton { label: qsTr("Monitorização"); glyph: "◔"; active: win.page === 3; onClicked: win.page = 3 }
                NavButton { label: qsTr("Definições"); glyph: "⚙"; active: win.page === 4; onClicked: win.page = 4 }
                Item { Layout.fillHeight: true }
                Text {
                    Layout.margins: 18
                    text: (info.product || "") + "\n" + qsTr("Versão ") + (info.version || "—")
                    color: Theme.textFaint
                    font.pixelSize: 11
                    lineHeight: 1.3
                }
            }
        }

        // ---------------------------------------------------------- content
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // header
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 64
                color: Theme.bg
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.line }
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 28
                    anchors.rightMargin: 28
                    spacing: 18
                    Text {
                        text: [qsTr("Início"), qsTr("Ventoinhas"), qsTr("Iluminação"), qsTr("Monitorização"), qsTr("Definições")][win.page]
                        color: Theme.text; font.pixelSize: 20; font.weight: Font.DemiBold
                    }
                    Item { Layout.fillWidth: true }
                    Row {
                        spacing: 8
                        visible: backend.connected
                        Text { text: st.onAC ? "⚡" : "🔋"; font.pixelSize: 14; color: Theme.textDim; anchors.verticalCenter: parent.verticalCenter }
                        Text { text: st.onAC ? qsTr("Carregador ligado") : qsTr("Na bateria"); color: Theme.textDim; font.pixelSize: 13; anchors.verticalCenter: parent.verticalCenter }
                    }
                    Rectangle {
                        visible: backend.connected && st.mode !== undefined
                        radius: 14
                        height: 28
                        width: modeLbl.implicitWidth + 28
                        color: Qt.rgba(Theme.modeColor(st.mode).r, Theme.modeColor(st.mode).g, Theme.modeColor(st.mode).b, 0.16)
                        border.color: Theme.modeColor(st.mode)
                        Text { id: modeLbl; anchors.centerIn: parent; text: backend.modeName(st.mode); color: Theme.text; font.pixelSize: 13; font.weight: Font.DemiBold }
                    }
                    BusyIndicator { running: backend.busy; visible: running; implicitWidth: 26; implicitHeight: 26 }
                }
            }

            // service not reachable
            Rectangle {
                visible: !backend.connected
                Layout.fillWidth: true
                Layout.preferredHeight: 44
                color: "#3a1414"
                Text {
                    anchors.centerIn: parent
                    text: qsTr("Serviço predator-sensed não está a correr. Ative-o com: sudo systemctl enable --now predator-sensed")
                    color: "#ffd0d0"; font.pixelSize: 13
                }
            }

            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: win.page
                enabled: backend.connected
                HomePage {}
                FanPage {}
                LightingPage {}
                MonitorPage {}
                SettingsPage {}
            }
        }
    }

    // error toast
    Rectangle {
        id: toast
        property string message
        function show(m) { message = m; opacity = 1; hideTimer.restart() }
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 26
        width: Math.min(parent.width - 80, toastText.implicitWidth + 40)
        height: toastText.implicitHeight + 22
        radius: Theme.radius
        color: "#2a1215"
        border.color: Theme.turbo
        opacity: 0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: 200 } }
        Text { id: toastText; anchors.centerIn: parent; text: toast.message; color: Theme.text; font.pixelSize: 13; width: Math.min(implicitWidth, win.width - 120); wrapMode: Text.WordWrap }
        Timer { id: hideTimer; interval: 5000; onTriggered: toast.opacity = 0 }
    }
}
