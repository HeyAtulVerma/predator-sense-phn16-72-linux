import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ScrollView {
    id: page
    contentWidth: availableWidth
    clip: true
    readonly property var info: backend.info
    readonly property var st: backend.state
    readonly property var oc: (info.gpu && info.gpu.oc) || ({})

    component Divider: Rectangle { Layout.fillWidth: true; height: 1; color: Theme.line }

    ColumnLayout {
        width: page.availableWidth
        spacing: Theme.gap
        Item { Layout.preferredHeight: 8 }

        Card {
            Layout.fillWidth: true
            Layout.leftMargin: 28; Layout.rightMargin: 28
            visible: info.battery === true || info.usbCharging === true
            title: qsTr("Bateria e carregamento")
            ColumnLayout {
                width: parent.width
                spacing: 14
                OptionRow {
                    visible: info.battery === true
                    title: qsTr("Limite de carga a 80%")
                    description: qsTr("Prolonga a vida útil da bateria quando o portátil fica muito tempo ligado à corrente (Battery Health Mode).")
                    PSwitch { checked: st.batteryLimit === true; onToggled: backend.setOption("batteryLimit", checked) }
                }
                Divider { visible: info.battery === true }
                OptionRow {
                    visible: info.battery === true
                    title: qsTr("Calibração da bateria")
                    description: st.batteryCalibration ? qsTr("Calibração em curso: mantenha o carregador ligado. O processo descarrega e carrega a bateria por completo e pode demorar várias horas.")
                                                       : qsTr("Recalibra o indicador de carga. Requer o carregador ligado durante todo o processo.")
                    PSwitch { checked: st.batteryCalibration === true; onToggled: backend.setOption("batteryCalibration", checked) }
                }
                Divider { visible: info.battery === true && info.usbCharging === true }
                OptionRow {
                    visible: info.usbCharging === true
                    title: qsTr("Carregamento USB com o portátil desligado")
                    description: qsTr("Mantém a porta USB de carregamento ativa com o sistema desligado.")
                    PSwitch { checked: st.usbCharging === true; onToggled: backend.setOption("usbCharging", checked) }
                }
                OptionRow {
                    visible: info.usbCharging === true
                    enabled: st.usbCharging === true
                    opacity: enabled ? 1 : 0.45
                    title: qsTr("Parar o carregamento USB quando a bateria descer abaixo de")
                    Row {
                        spacing: 8
                        Repeater {
                            model: [10, 20, 30]
                            SegButton { implicitWidth: 64; text: modelData + "%"; selected: st.usbChargingThreshold === modelData; onClicked: backend.setOption("usbChargingThreshold", modelData) }
                        }
                    }
                }
            }
        }

        Card {
            Layout.fillWidth: true
            Layout.leftMargin: 28; Layout.rightMargin: 28
            title: qsTr("Sistema")
            ColumnLayout {
                width: parent.width
                spacing: 14
                OptionRow {
                    visible: info.bootSound === true
                    title: qsTr("Animação e som de arranque")
                    description: qsTr("Mostra a animação Predator com som ao ligar o portátil.")
                    PSwitch { checked: st.bootSound === true; onToggled: backend.setOption("bootSound", checked) }
                }
                OptionRow {
                    visible: info.lcdOverdrive === true
                    title: qsTr("LCD Overdrive")
                    description: qsTr("Reduz o tempo de resposta do ecrã.")
                    PSwitch { checked: st.lcdOverdrive === true; onToggled: backend.setOption("lcdOverdrive", checked) }
                }
                OptionRow {
                    visible: info.keyboard && info.keyboard.timeout === true
                    title: qsTr("Desligar a retroiluminação do teclado após 30 s")
                    PSwitch { checked: st.kbTimeout === true; onToggled: backend.setOption("kbTimeout", checked) }
                }
                OptionRow {
                    visible: info.gpu && info.gpu.present && page.oc.core !== undefined
                    title: qsTr("Overclock da GPU por modo (perfil Acer)")
                    description: page.oc.core ? qsTr("Valores do PredatorSense para esta GPU — Turbo: +%1 MHz núcleo, +%2 MHz memória. Aplicado via NVML.").arg(page.oc.core[3]).arg(page.oc.mem[3]) : ""
                    PSwitch { checked: st.gpuOcInModes !== false; onToggled: backend.setOption("gpuOcInModes", checked) }
                }
            }
        }

        Card {
            Layout.fillWidth: true
            Layout.leftMargin: 28; Layout.rightMargin: 28
            title: qsTr("Aplicação")
            ColumnLayout {
                width: parent.width
                spacing: 14
                OptionRow {
                    title: qsTr("Fechar para a bandeja do sistema")
                    description: qsTr("A janela fecha mas o ícone continua na bandeja para trocar de modo rapidamente.")
                    PSwitch { checked: backend.closeToTray; onToggled: backend.closeToTray = checked }
                }
                OptionRow {
                    title: qsTr("Temperatura em Fahrenheit")
                    PSwitch { checked: backend.fahrenheit; onToggled: backend.fahrenheit = checked }
                }
            }
        }

        Card {
            Layout.fillWidth: true
            Layout.leftMargin: 28; Layout.rightMargin: 28
            title: qsTr("Sobre")
            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                color: Theme.textDim
                font.pixelSize: 12
                lineHeight: 1.35
                text: qsTr("Modelo: %1\nGPU: %2\nVentoinhas: %3 CPU, %4 GPU · Teclado: %5 zonas\n\nPredator Sense para Linux reimplementa o protocolo do PredatorSense 5.0.1463 da Acer (engenharia reversa para interoperabilidade). Projeto independente, sem afiliação à Acer.")
                      .arg(info.product || "—").arg(info.gpu ? info.gpu.pciId : "—")
                      .arg(info.fans ? info.fans.cpu : 0).arg(info.fans ? info.fans.gpu : 0)
                      .arg(info.keyboard ? info.keyboard.zones : "—")
            }
        }
        Item { Layout.preferredHeight: 12 }
    }
}
