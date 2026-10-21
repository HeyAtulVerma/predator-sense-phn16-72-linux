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
    readonly property var fan: st.fan || ({})
    readonly property string fanMode: fan.mode || "auto"
    readonly property bool quietLock: st.mode === 0

    function fanSetting(isGpu, idx) {
        const f = isGpu ? ((fan.gpu || [])[idx] || {}) : (fan.cpu || {})
        return { auto: f.auto !== false, speed: f.speed !== undefined ? f.speed : 50 }
    }

    function applyCustom(isGpu, idx, auto, speed) {
        const gpu = []
        for (let i = 0; i < (info.fans ? info.fans.gpu : 0); ++i)
            gpu.push(fanSetting(true, i))
        let cpu = fanSetting(false, 0)
        if (isGpu) gpu[idx] = { auto: auto, speed: speed }
        else cpu = { auto: auto, speed: speed }
        backend.setFan({ mode: "custom", cpu: cpu, gpu: gpu })
    }

    component FanCard: Card {
        id: fc
        property bool isGpu: false
        property int index: 0
        property string name
        property var rpm
        readonly property var cfg: page.fanSetting(isGpu, index)
        Layout.fillWidth: true
        Layout.preferredWidth: 1
        title: name
        ColumnLayout {
            width: parent.width
            spacing: 14
            RowLayout {
                Layout.fillWidth: true
                Text { text: fc.rpm !== undefined ? fc.rpm + " rpm" : "—"; color: Theme.accent; font.pixelSize: 28; font.weight: Font.DemiBold }
                Item { Layout.fillWidth: true }
                Text { text: qsTr("Automático"); color: Theme.textDim; font.pixelSize: 13 }
                PSwitch {
                    checked: fc.cfg.auto
                    onToggled: page.applyCustom(fc.isGpu, fc.index, checked, slider.value)
                }
            }
            RowLayout {
                Layout.fillWidth: true
                enabled: !fc.cfg.auto
                opacity: enabled ? 1 : 0.4
                Text { text: "0%"; color: Theme.textFaint; font.pixelSize: 11 }
                PSlider {
                    id: slider
                    Layout.fillWidth: true
                    from: 0; to: 100; stepSize: 5
                    value: fc.cfg.speed
                    onPressedChanged: if (!pressed) page.applyCustom(fc.isGpu, fc.index, false, value)
                }
                Text { text: "100%"; color: Theme.textFaint; font.pixelSize: 11 }
                Text { Layout.preferredWidth: 52; horizontalAlignment: Text.AlignRight; text: Math.round(slider.value) + "%"; color: Theme.text; font.pixelSize: 16; font.weight: Font.DemiBold }
            }
        }
    }

    ColumnLayout {
        width: page.availableWidth
        spacing: Theme.gap
        Item { Layout.preferredHeight: 8 }

        Card {
            Layout.fillWidth: true
            Layout.leftMargin: 28; Layout.rightMargin: 28
            title: qsTr("Controlo das ventoinhas")
            subtitle: page.quietLock ? qsTr("No modo Silencioso as ventoinhas ficam em Automático (comportamento do PredatorSense).")
                                     : qsTr("Automático segue a curva do firmware. Máximo coloca todas as ventoinhas na velocidade máxima. Personalizado define a velocidade de cada uma.")
            Row {
                spacing: 10
                SegButton { text: qsTr("Automático"); selected: page.fanMode === "auto"; onClicked: backend.setFan({ mode: "auto" }) }
                SegButton { text: qsTr("Máximo"); selected: page.fanMode === "max"; tint: Theme.turbo; enabled: !page.quietLock; onClicked: backend.setFan({ mode: "max" }) }
                SegButton { text: qsTr("Personalizado"); selected: page.fanMode === "custom"; enabled: !page.quietLock; onClicked: backend.setFan({ mode: "custom" }) }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 28; Layout.rightMargin: 28
            columns: 2
            columnSpacing: Theme.gap
            rowSpacing: Theme.gap
            visible: page.fanMode === "custom"
            FanCard { name: qsTr("Ventoinha da CPU"); isGpu: false; rpm: sn.cpuFan; visible: info.fans && info.fans.cpu > 0 }
            FanCard { name: qsTr("Ventoinha da GPU"); isGpu: true; index: 0; rpm: sn.gpu1Fan; visible: info.fans && info.fans.gpu > 0 }
            FanCard { name: qsTr("Ventoinha da GPU 2"); isGpu: true; index: 1; rpm: sn.gpu2Fan; visible: info.fans && info.fans.gpu > 1 }
        }

        Card {
            Layout.fillWidth: true
            Layout.leftMargin: 28; Layout.rightMargin: 28
            visible: page.fanMode !== "custom"
            title: qsTr("Velocidade atual")
            Row {
                spacing: 40
                Column {
                    Text { text: qsTr("CPU"); color: Theme.textDim; font.pixelSize: 12 }
                    Text { text: sn.cpuFan !== undefined ? sn.cpuFan + " rpm" : "—"; color: Theme.text; font.pixelSize: 26; font.weight: Font.DemiBold }
                }
                Column {
                    Text { text: qsTr("GPU"); color: Theme.textDim; font.pixelSize: 12 }
                    Text { text: sn.gpu1Fan !== undefined ? sn.gpu1Fan + " rpm" : "—"; color: Theme.text; font.pixelSize: 26; font.weight: Font.DemiBold }
                }
            }
        }
        Item { Layout.preferredHeight: 12 }
    }
}
