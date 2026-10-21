import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ScrollView {
    id: page
    contentWidth: availableWidth
    clip: true
    readonly property var sn: backend.sensors

    component Legend: Row {
        property var items: []
        spacing: 18
        Repeater {
            model: parent.items
            Row {
                spacing: 6
                Rectangle { width: 10; height: 10; radius: 5; color: modelData.color; anchors.verticalCenter: parent.verticalCenter }
                Text { text: modelData.label; color: Theme.textDim; font.pixelSize: 12 }
            }
        }
    }
    component Stat: Column {
        property string label
        property string value
        spacing: 2
        Text { text: parent.label; color: Theme.textDim; font.pixelSize: 12 }
        Text { text: parent.value; color: Theme.text; font.pixelSize: 20; font.weight: Font.DemiBold }
    }

    ColumnLayout {
        width: page.availableWidth
        spacing: Theme.gap
        Item { Layout.preferredHeight: 8 }

        GridLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 28; Layout.rightMargin: 28
            columns: 2
            rowSpacing: Theme.gap; columnSpacing: Theme.gap

            Card {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                title: qsTr("Temperaturas")
                ColumnLayout {
                    width: parent.width
                    Legend { items: [{ label: "CPU", color: Theme.accent }, { label: "GPU", color: Theme.warn }] }
                    LineChart {
                        Layout.fillWidth: true
                        minimum: 20; maximum: 105; unit: "°"
                        revision: history.revision
                        series: [{ data: history.cpuTemp, color: Theme.accent }, { data: history.gpuTemp, color: Theme.warn }]
                    }
                }
            }
            Card {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                title: qsTr("Ventoinhas")
                ColumnLayout {
                    width: parent.width
                    Legend { items: [{ label: "CPU", color: Theme.accent }, { label: "GPU", color: Theme.warn }] }
                    LineChart {
                        Layout.fillWidth: true
                        minimum: 0; maximum: 7500
                        revision: history.revision
                        series: [{ data: history.cpuFan, color: Theme.accent }, { data: history.gpuFan, color: Theme.warn }]
                    }
                }
            }
            Card {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                title: qsTr("Utilização")
                ColumnLayout {
                    width: parent.width
                    Legend { items: [{ label: "CPU", color: Theme.accent }, { label: "GPU", color: Theme.warn }] }
                    LineChart {
                        Layout.fillWidth: true
                        minimum: 0; maximum: 100; unit: "%"
                        revision: history.revision
                        series: [{ data: history.cpuUsage, color: Theme.accent }, { data: history.gpuUsage, color: Theme.warn }]
                    }
                }
            }
            Card {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                title: qsTr("Detalhes")
                GridLayout {
                    width: parent.width
                    columns: 2
                    rowSpacing: 18; columnSpacing: 24
                    Stat { label: qsTr("CPU frequência"); value: sn.cpuFreq ? (sn.cpuFreq / 1000).toFixed(2) + " GHz" : "—" }
                    Stat { label: qsTr("CPU uso"); value: sn.cpuUsage !== undefined ? Math.round(sn.cpuUsage) + "%" : "—" }
                    Stat { label: qsTr("Sistema"); value: sn.sysTemp !== undefined ? win.temp(sn.sysTemp) + win.tempUnit : "—" }
                    Stat { label: qsTr("Memória"); value: sn.ramTotal ? Math.round(sn.ramUsage) + "% de " + sn.ramTotal.toFixed(1) + " GB" : "—" }
                    Stat { label: qsTr("GPU"); value: sn.gpuIdle ? qsTr("em repouso") : (sn.gpuUsage !== undefined ? sn.gpuUsage + "%" : "—") }
                    Stat { label: qsTr("GPU núcleo / memória"); value: sn.gpuIdle || sn.gpuCoreClock === undefined ? "—" : sn.gpuCoreClock + " / " + sn.gpuMemClock + " MHz" }
                    Stat { label: qsTr("GPU potência"); value: sn.gpuIdle || sn.gpuPower === undefined ? "—" : sn.gpuPower.toFixed(1) + " W" }
                }
            }
        }
        Text {
            Layout.leftMargin: 28; Layout.rightMargin: 28
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("Temperaturas e rotações vêm do EC (GetGamingSysInfo), como no PredatorSense. Enquanto a GPU dedicada estiver em repouso não é acordada só para ler números.")
            color: Theme.textFaint; font.pixelSize: 12
        }
        Item { Layout.preferredHeight: 12 }
    }
}
