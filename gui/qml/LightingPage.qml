import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ScrollView {
    id: page
    contentWidth: availableWidth
    clip: true

    readonly property var info: backend.info
    readonly property var kb: info.keyboard || ({})
    readonly property var light: backend.state.lighting || ({})
    readonly property int zoneCount: kb.zones > 0 ? kb.zones : 4
    readonly property bool supported: kb.zones !== 0

    // Local editing copy, pushed to the daemon after a short debounce.
    property string effect: "static"
    property var zones: ["#00aec7", "#00aec7", "#00aec7", "#00aec7"]
    property color color: "#00aec7"
    property int speed: 5
    property int brightness: 100
    property int direction: 1
    property int selectedZone: -1     // -1 = all zones
    property bool loaded: false

    function load() {
        if (!light.effect) return
        effect = light.effect
        zones = (light.zones || zones).slice()
        color = light.color || color
        speed = light.speed || 5
        brightness = light.brightness !== undefined ? light.brightness : 100
        direction = light.direction || 1
        loaded = true
    }
    Connections { target: backend; function onStateChanged() { if (!pushTimer.running && !page.dirty) page.load() } }
    Component.onCompleted: load()

    property bool dirty: false
    function push() { dirty = true; pushTimer.restart() }
    Timer {
        id: pushTimer
        interval: 250
        onTriggered: {
            page.dirty = false
            backend.setLighting({ effect: page.effect, zones: page.zones, color: page.color.toString(),
                                  speed: page.speed, brightness: page.brightness, direction: page.direction })
        }
    }

    readonly property var effects: [
        { id: "static", name: qsTr("Estático"), color: false, dir: false },
        { id: "breathing", name: qsTr("Respiração"), color: true, dir: false },
        { id: "neon", name: qsTr("Néon"), color: false, dir: false },
        { id: "wave", name: qsTr("Onda"), color: false, dir: true },
        { id: "shifting", name: qsTr("Deslocamento"), color: true, dir: true },
        { id: "zoom", name: qsTr("Zoom"), color: true, dir: false },
        { id: "meteor", name: qsTr("Meteoro"), color: true, dir: false },
        { id: "twinkling", name: qsTr("Cintilação"), color: true, dir: false }
    ]
    readonly property var current: effects.find(e => e.id === effect) || effects[0]

    // ---------------------------------------------------------- keyboard preview
    component KeyboardPreview: Item {
        id: kbp
        implicitHeight: 230
        property real phase: 0
        NumberAnimation on phase { from: 0; to: 1; duration: 4000 - page.speed * 350; loops: Animation.Infinite; running: page.effect !== "static" }

        function zoneColor(z) {
            if (page.brightness === 0) return "#101a1e"
            const k = page.brightness / 100
            let c
            switch (page.effect) {
            case "static": c = Qt.color(page.zones[z] || "#000000"); break
            case "neon": c = Qt.hsva((kbp.phase + z * 0.05) % 1, 1, 1, 1); break
            case "wave": {
                const p = page.direction === 2 ? z / page.zoneCount : 1 - z / page.zoneCount
                c = Qt.hsva((kbp.phase + p) % 1, 1, 1, 1); break
            }
            case "breathing": { const b = 0.15 + 0.85 * Math.abs(Math.sin(kbp.phase * Math.PI)); c = Qt.hsva(page.color.hsvHue, page.color.hsvSaturation, page.color.hsvValue * b, 1); break }
            default: { const on = Math.abs(Math.sin((kbp.phase * page.zoneCount - z) * Math.PI)); c = Qt.hsva(Math.max(0, page.color.hsvHue), page.color.hsvSaturation, page.color.hsvValue * (0.25 + 0.75 * on), 1) }
            }
            return Qt.hsva(Math.max(0, c.hsvHue), c.hsvSaturation, c.hsvValue * (0.25 + 0.75 * k), 1)
        }

        Rectangle {
            id: chassis
            anchors.centerIn: parent
            width: Math.min(parent.width, 760)
            height: 220
            radius: 10
            color: "#0a1418"
            border.color: Theme.line
            Column {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 5
                Repeater {
                    model: 6
                    Row {
                        id: keyRow
                        property int r: index
                        spacing: 5
                        // relative key widths per row (last row has the space bar)
                        readonly property var units: [
                            [1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1],
                            [1,1,1,1,1,1,1,1,1,1,1,1,1,2,1],
                            [1.5,1,1,1,1,1,1,1,1,1,1,1,1,1.5,1],
                            [1.8,1,1,1,1,1,1,1,1,1,1,1,2.2,1],
                            [2.3,1,1,1,1,1,1,1,1,1,1,2.7,1],
                            [1.3,1,1.3,1.3,6,1.3,1,1,1]
                        ][r]
                        readonly property real total: units.reduce((a, b) => a + b, 0)
                        readonly property real unitW: (chassis.width - 28 - (units.length - 1) * 5) / total
                        Repeater {
                            model: keyRow.units.length
                            Rectangle {
                                readonly property int zone: Math.min(page.zoneCount - 1, Math.floor((x + width / 2) / (chassis.width - 28) * page.zoneCount))
                                width: keyRow.unitW * keyRow.units[index]
                                height: (chassis.height - 28 - 25) / 6
                                radius: 3
                                color: "#121e23"
                                border.color: kbp.zoneColor(zone)
                                border.width: 1
                                Rectangle {
                                    anchors.fill: parent; anchors.margins: 2; radius: 2
                                    color: kbp.zoneColor(parent.zone); opacity: 0.55
                                }
                            }
                        }
                    }
                }
            }
            // zone selector overlay (static effect)
            Row {
                anchors.fill: parent
                visible: page.effect === "static"
                Repeater {
                    model: page.zoneCount
                    Rectangle {
                        width: chassis.width / page.zoneCount; height: chassis.height
                        color: "transparent"
                        border.color: page.selectedZone === index ? "white" : "transparent"
                        border.width: 2
                        radius: 8
                        Text { anchors.top: parent.top; anchors.horizontalCenter: parent.horizontalCenter; anchors.topMargin: -22; text: qsTr("Zona %1").arg(index + 1); color: page.selectedZone === index ? Theme.text : Theme.textDim; font.pixelSize: 12 }
                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: page.selectedZone = (page.selectedZone === index ? -1 : index) }
                    }
                }
            }
        }
    }

    ColumnLayout {
        width: page.availableWidth
        spacing: Theme.gap
        Item { Layout.preferredHeight: 8 }

        Card {
            visible: !page.supported
            Layout.fillWidth: true
            Layout.leftMargin: 28; Layout.rightMargin: 28
            title: qsTr("Teclado não suportado")
            subtitle: qsTr("Este modelo tem teclado RGB por tecla (USB). Por agora só os teclados RGB por zonas controlados pelo EC são suportados.")
        }

        Card {
            visible: page.supported
            Layout.fillWidth: true
            Layout.leftMargin: 28; Layout.rightMargin: 28
            title: qsTr("Teclado RGB · %1 zonas").arg(page.zoneCount)
            subtitle: page.effect === "static" ? qsTr("Clique numa zona para a editar sozinha; sem zona selecionada a cor vai para todas.") : ""
            ColumnLayout {
                width: parent.width
                spacing: 18
                Item { Layout.preferredHeight: 14 }
                KeyboardPreview { Layout.fillWidth: true }
            }
        }

        RowLayout {
            visible: page.supported
            Layout.fillWidth: true
            Layout.leftMargin: 28; Layout.rightMargin: 28
            spacing: Theme.gap

            Card {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                title: qsTr("Efeito")
                ColumnLayout {
                    width: parent.width
                    spacing: 16
                    GridLayout {
                        columns: 4
                        rowSpacing: 8; columnSpacing: 8
                        Repeater {
                            model: page.effects
                            SegButton {
                                Layout.fillWidth: true
                                text: modelData.name
                                selected: page.effect === modelData.id
                                onClicked: { page.effect = modelData.id; page.push() }
                            }
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: qsTr("Brilho"); color: Theme.textDim; font.pixelSize: 13; Layout.preferredWidth: 90 }
                        PSlider { Layout.fillWidth: true; from: 0; to: 100; stepSize: 5; value: page.brightness; onMoved: page.brightness = value; onPressedChanged: if (!pressed) page.push() }
                        Text { text: page.brightness + "%"; color: Theme.text; font.pixelSize: 13; Layout.preferredWidth: 44; horizontalAlignment: Text.AlignRight }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        visible: page.effect !== "static"
                        Text { text: qsTr("Velocidade"); color: Theme.textDim; font.pixelSize: 13; Layout.preferredWidth: 90 }
                        PSlider { Layout.fillWidth: true; from: 1; to: 9; stepSize: 1; snapMode: Slider.SnapAlways; value: page.speed; onMoved: page.speed = value; onPressedChanged: if (!pressed) page.push() }
                        Text { text: page.speed; color: Theme.text; font.pixelSize: 13; Layout.preferredWidth: 44; horizontalAlignment: Text.AlignRight }
                    }
                    RowLayout {
                        visible: page.current.dir
                        Text { text: qsTr("Direção"); color: Theme.textDim; font.pixelSize: 13; Layout.preferredWidth: 90 }
                        SegButton { text: qsTr("◀ Para a esquerda"); selected: page.direction === 2; onClicked: { page.direction = 2; page.push() } }
                        SegButton { text: qsTr("Para a direita ▶"); selected: page.direction === 1; onClicked: { page.direction = 1; page.push() } }
                    }
                }
            }

            Card {
                Layout.preferredWidth: 380
                Layout.alignment: Qt.AlignTop
                visible: page.effect === "static" || page.current.color
                title: page.effect === "static" ? (page.selectedZone < 0 ? qsTr("Cor · todas as zonas") : qsTr("Cor · zona %1").arg(page.selectedZone + 1)) : qsTr("Cor")
                ColorPicker {
                    width: parent.width
                    color: page.effect === "static" ? (page.zones[Math.max(0, page.selectedZone)] || "#00aec7") : page.color
                    onPicked: (c) => {
                        if (page.effect === "static") {
                            const z = page.zones.slice()
                            for (let i = 0; i < page.zoneCount; ++i)
                                if (page.selectedZone < 0 || page.selectedZone === i) z[i] = c.toString()
                            page.zones = z
                        } else {
                            page.color = c
                        }
                        page.push()
                    }
                }
            }
        }

        Card {
            visible: page.supported && kb.timeout === true
            Layout.fillWidth: true
            Layout.leftMargin: 28; Layout.rightMargin: 28
            OptionRow {
                width: parent.width
                title: qsTr("Desligar a retroiluminação após 30 s sem uso")
                description: qsTr("O firmware apaga o teclado depois de 30 segundos de inatividade.")
                PSwitch { checked: backend.state.kbTimeout === true; onToggled: backend.setOption("kbTimeout", checked) }
            }
        }
        Item { Layout.preferredHeight: 12 }
    }
}
