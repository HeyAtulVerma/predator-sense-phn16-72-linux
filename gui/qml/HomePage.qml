import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ScrollView {
    id: page
    contentWidth: availableWidth
    clip: true

    readonly property var info: backend.info
    readonly property var st: backend.state
    readonly property var sn: backend.sensors
    readonly property var modes: info.modes || []

    function modeDescription(m) {
        var _ = backend.language
        switch (m) {
        case 0: return qsTr("Silencioso, para tarefas leves")
        case 1: return qsTr("Equilíbrio entre ruído e desempenho")
        case 4: return qsTr("Mais potência para jogos")
        case 5: return qsTr("Máximo desempenho e arrefecimento")
        case 6: return qsTr("Poupança de energia")
        }
        return ""
    }
    function modeAllowed(m) { return st.onAC || m === 1 || m === 6 }

    component ModeCard: Rectangle {
        id: card
        property int mode
        readonly property bool current: st.mode === mode
        readonly property color tint: Theme.modeColor(mode)
        Layout.fillWidth: true
        Layout.preferredHeight: 150
        radius: Theme.radius
        color: current ? Qt.rgba(tint.r, tint.g, tint.b, 0.14) : (ma.containsMouse ? Theme.panelHi : Theme.panel)
        border.color: current ? tint : Theme.line
        border.width: current ? 2 : 1
        opacity: page.modeAllowed(mode) ? 1 : 0.38
        Behavior on color { ColorAnimation { duration: 150 } }
        Column {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 6
            Rectangle { width: 30; height: 4; radius: 2; color: card.tint }
            Item { width: 1; height: 8 }
            Text { text: backend.modeName(card.mode); color: Theme.text; font.pixelSize: 19; font.weight: Font.DemiBold }
            Text { width: parent.width; text: page.modeDescription(card.mode); color: Theme.textDim; font.pixelSize: 12; wrapMode: Text.WordWrap }
        }
        Text {
            anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.margins: 12
            visible: !page.modeAllowed(card.mode)
            text: qsTr("requer carregador"); color: Theme.textDim; font.pixelSize: 11
        }
        MouseArea {
            id: ma
            anchors.fill: parent
            hoverEnabled: true
            enabled: page.modeAllowed(card.mode) && !card.current
            cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
            onClicked: backend.setMode(card.mode)
        }
    }

    ColumnLayout {
        width: page.availableWidth
        spacing: Theme.gap

        Item { Layout.preferredHeight: 8 }

        Card {
            Layout.fillWidth: true
            Layout.leftMargin: 28; Layout.rightMargin: 28
            title: qsTr("Modo de funcionamento")
            subtitle: st.onAC ? qsTr("Ao mudar de modo, o firmware ajusta limites de energia, ventoinhas e, em Turbo, aplica o overclock de GPU definido pela Acer para este modelo.")
                              : qsTr("Na bateria o PredatorSense só permite Equilibrado ou Eco. Ligue o carregador para os modos de desempenho.")
            RowLayout {
                width: parent.width
                spacing: 12
                Repeater {
                    model: page.modes
                    ModeCard { mode: modelData.value }
                }
            }
        }

        Card {
            Layout.fillWidth: true
            Layout.leftMargin: 28; Layout.rightMargin: 28
            title: qsTr("Modo na bateria")
            subtitle: qsTr("Modo aplicado automaticamente ao desligar o carregador. Ao voltar a ligá-lo, regressa ao modo escolhido acima.")
            Row {
                spacing: 10
                Repeater {
                    model: (info.batteryModes || []).filter(m => page.modes.some(x => x.value === m))
                    SegButton {
                        text: backend.modeName(modelData)
                        selected: st.dcMode === modelData
                        tint: Theme.modeColor(modelData)
                        onClicked: backend.setBatteryMode(modelData)
                    }
                }
            }
        }

        Card {
            Layout.fillWidth: true
            Layout.leftMargin: 28; Layout.rightMargin: 28
            title: qsTr("Estado do sistema")
            GridLayout {
                width: parent.width
                columns: 6
                columnSpacing: 10
                Gauge {
                    Layout.alignment: Qt.AlignHCenter; Layout.preferredWidth: 140; Layout.preferredHeight: 140
                    label: qsTr("CPU"); value: sn.cpuTemp || 0; maximum: 105; available: sn.cpuTemp !== undefined
                    valueText: win.temp(sn.cpuTemp); unit: win.tempUnit; arcColor: Theme.tempColor(sn.cpuTemp || 0)
                }
                Gauge {
                    Layout.alignment: Qt.AlignHCenter; Layout.preferredWidth: 140; Layout.preferredHeight: 140
                    label: qsTr("GPU"); value: sn.gpu1Temp || 0; maximum: 95; available: sn.gpu1Temp !== undefined
                    valueText: win.temp(sn.gpu1Temp); unit: win.tempUnit; arcColor: Theme.tempColor(sn.gpu1Temp || 0)
                }
                Gauge {
                    Layout.alignment: Qt.AlignHCenter; Layout.preferredWidth: 140; Layout.preferredHeight: 140
                    label: qsTr("Sistema"); value: sn.sysTemp || 0; maximum: 90; available: sn.sysTemp !== undefined
                    valueText: win.temp(sn.sysTemp); unit: win.tempUnit; arcColor: Theme.tempColor(sn.sysTemp || 0)
                }
                Gauge {
                    Layout.alignment: Qt.AlignHCenter; Layout.preferredWidth: 140; Layout.preferredHeight: 140
                    label: qsTr("Ventoinha CPU"); value: sn.cpuFan || 0; maximum: 7500; available: sn.cpuFan !== undefined
                    unit: " rpm"
                }
                Gauge {
                    Layout.alignment: Qt.AlignHCenter; Layout.preferredWidth: 140; Layout.preferredHeight: 140
                    label: qsTr("Ventoinha GPU"); value: sn.gpu1Fan || 0; maximum: 7500; available: sn.gpu1Fan !== undefined
                    unit: " rpm"
                }
                Gauge {
                    Layout.alignment: Qt.AlignHCenter; Layout.preferredWidth: 140; Layout.preferredHeight: 140
                    label: qsTr("Uso CPU"); value: sn.cpuUsage || 0; maximum: 100; unit: "%"
                }
            }
        }

        Item { Layout.preferredHeight: 12 }
    }
}
