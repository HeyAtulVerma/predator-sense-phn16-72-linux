#include "Translator.h"

AppTranslator::AppTranslator(QObject *parent)
    : QTranslator(parent)
{
    // General & Navigation
    m_dict.insert(QStringLiteral("Início"), QStringLiteral("Home"));
    m_dict.insert(QStringLiteral("Ventoinhas"), QStringLiteral("Fans"));
    m_dict.insert(QStringLiteral("Iluminação"), QStringLiteral("Lighting"));
    m_dict.insert(QStringLiteral("Monitorização"), QStringLiteral("Monitoring"));
    m_dict.insert(QStringLiteral("Definições"), QStringLiteral("Settings"));
    m_dict.insert(QStringLiteral("Versão "), QStringLiteral("Version "));
    m_dict.insert(QStringLiteral("Carregador ligado"), QStringLiteral("Charger connected"));
    m_dict.insert(QStringLiteral("Na bateria"), QStringLiteral("On battery"));
    m_dict.insert(QStringLiteral("Serviço predator-sensed não está a correr. Ative-o com: sudo systemctl enable --now predator-sensed"),
                  QStringLiteral("predator-sensed service is not running. Enable it with: sudo systemctl enable --now predator-sensed"));

    // Tray Menu
    m_dict.insert(QStringLiteral("Abrir Predator Sense"), QStringLiteral("Open Predator Sense"));
    m_dict.insert(QStringLiteral("Sair"), QStringLiteral("Quit"));

    // Operating Modes
    m_dict.insert(QStringLiteral("Silencioso"), QStringLiteral("Quiet"));
    m_dict.insert(QStringLiteral("Equilibrado"), QStringLiteral("Balanced"));
    m_dict.insert(QStringLiteral("Desempenho"), QStringLiteral("Performance"));
    m_dict.insert(QStringLiteral("Turbo"), QStringLiteral("Turbo"));
    m_dict.insert(QStringLiteral("Eco"), QStringLiteral("Eco"));
    m_dict.insert(QStringLiteral("Desconhecido"), QStringLiteral("Unknown"));

    // Mode Descriptions (HomePage)
    m_dict.insert(QStringLiteral("Silencioso, para tarefas leves"), QStringLiteral("Quiet, for light tasks"));
    m_dict.insert(QStringLiteral("Equilíbrio entre ruído e desempenho"), QStringLiteral("Balance between noise and performance"));
    m_dict.insert(QStringLiteral("Mais potência para jogos"), QStringLiteral("More power for gaming"));
    m_dict.insert(QStringLiteral("Máximo desempenho e arrefecimento"), QStringLiteral("Maximum performance and cooling"));
    m_dict.insert(QStringLiteral("Poupança de energia"), QStringLiteral("Power saving"));
    m_dict.insert(QStringLiteral("requer carregador"), QStringLiteral("requires charger"));

    m_dict.insert(QStringLiteral("Modo de funcionamento"), QStringLiteral("Operating mode"));
    m_dict.insert(QStringLiteral("Ao mudar de modo, o firmware ajusta limites de energia, ventoinhas e, em Turbo, aplica o overclock de GPU definido pela Acer para este modelo."),
                  QStringLiteral("When changing modes, firmware adjusts power limits, fans and, in Turbo, applies Acer's GPU overclock for this model."));
    m_dict.insert(QStringLiteral("Na bateria o PredatorSense só permite Equilibrado ou Eco. Ligue o carregador para os modos de desempenho."),
                  QStringLiteral("On battery PredatorSense only allows Balanced or Eco. Connect the charger for performance modes."));

    m_dict.insert(QStringLiteral("Modo na bateria"), QStringLiteral("Battery mode"));
    m_dict.insert(QStringLiteral("Modo aplicado automaticamente ao desligar o carregador. Ao voltar a ligá-lo, regressa ao modo escolhido acima."),
                  QStringLiteral("Mode automatically applied when unplugging charger. When reconnecting, it returns to the mode chosen above."));

    // Gauges / System State (HomePage)
    m_dict.insert(QStringLiteral("Estado do sistema"), QStringLiteral("System status"));
    m_dict.insert(QStringLiteral("CPU"), QStringLiteral("CPU"));
    m_dict.insert(QStringLiteral("GPU"), QStringLiteral("GPU"));
    m_dict.insert(QStringLiteral("Sistema"), QStringLiteral("System"));
    m_dict.insert(QStringLiteral("Ventoinha CPU"), QStringLiteral("CPU Fan"));
    m_dict.insert(QStringLiteral("Ventoinha GPU"), QStringLiteral("GPU Fan"));
    m_dict.insert(QStringLiteral("Uso CPU"), QStringLiteral("CPU Usage"));

    // Fans (FanPage)
    m_dict.insert(QStringLiteral("Controlo das ventoinhas"), QStringLiteral("Fan control"));
    m_dict.insert(QStringLiteral("No modo Silencioso as ventoinhas ficam em Automático (comportamento do PredatorSense)."),
                  QStringLiteral("In Quiet mode fans remain on Auto (PredatorSense behavior)."));
    m_dict.insert(QStringLiteral("Automático segue a curva do firmware. Máximo coloca todas as ventoinhas na velocidade máxima. Personalizado define a velocidade de cada uma."),
                  QStringLiteral("Auto follows firmware curve. Max sets all fans to maximum speed. Custom sets the speed of each fan."));
    m_dict.insert(QStringLiteral("Automático"), QStringLiteral("Auto"));
    m_dict.insert(QStringLiteral("Máximo"), QStringLiteral("Max"));
    m_dict.insert(QStringLiteral("Personalizado"), QStringLiteral("Custom"));
    m_dict.insert(QStringLiteral("Ventoinha da CPU"), QStringLiteral("CPU Fan"));
    m_dict.insert(QStringLiteral("Ventoinha da GPU"), QStringLiteral("GPU Fan"));
    m_dict.insert(QStringLiteral("Ventoinha da GPU 2"), QStringLiteral("GPU 2 Fan"));
    m_dict.insert(QStringLiteral("Velocidade atual"), QStringLiteral("Current speed"));

    // Lighting (LightingPage)
    m_dict.insert(QStringLiteral("Estático"), QStringLiteral("Static"));
    m_dict.insert(QStringLiteral("Respiração"), QStringLiteral("Breathing"));
    m_dict.insert(QStringLiteral("Néon"), QStringLiteral("Neon"));
    m_dict.insert(QStringLiteral("Onda"), QStringLiteral("Wave"));
    m_dict.insert(QStringLiteral("Deslocamento"), QStringLiteral("Shifting"));
    m_dict.insert(QStringLiteral("Zoom"), QStringLiteral("Zoom"));
    m_dict.insert(QStringLiteral("Meteoro"), QStringLiteral("Meteor"));
    m_dict.insert(QStringLiteral("Cintilação"), QStringLiteral("Twinkling"));
    m_dict.insert(QStringLiteral("Zona %1"), QStringLiteral("Zone %1"));
    m_dict.insert(QStringLiteral("Teclado não suportado"), QStringLiteral("Keyboard not supported"));
    m_dict.insert(QStringLiteral("Este modelo tem teclado RGB por tecla (USB). Por agora só os teclados RGB por zonas controlados pelo EC são suportados."),
                  QStringLiteral("This model has a per-key RGB keyboard (USB). Currently only EC-controlled zone RGB keyboards are supported."));
    m_dict.insert(QStringLiteral("Teclado RGB · %1 zonas"), QStringLiteral("RGB Keyboard · %1 zones"));
    m_dict.insert(QStringLiteral("Clique numa zona para a editar sozinha; sem zona selecionada a cor vai para todas."),
                  QStringLiteral("Click a zone to edit it individually; without a zone selected the color applies to all."));
    m_dict.insert(QStringLiteral("Efeito"), QStringLiteral("Effect"));
    m_dict.insert(QStringLiteral("Brilho"), QStringLiteral("Brightness"));
    m_dict.insert(QStringLiteral("Velocidade"), QStringLiteral("Speed"));
    m_dict.insert(QStringLiteral("Direção"), QStringLiteral("Direction"));
    m_dict.insert(QStringLiteral("◀ Para a esquerda"), QStringLiteral("◀ To the left"));
    m_dict.insert(QStringLiteral("Para a direita ▶"), QStringLiteral("To the right ▶"));
    m_dict.insert(QStringLiteral("Cor · todas as zonas"), QStringLiteral("Color · all zones"));
    m_dict.insert(QStringLiteral("Cor · zona %1"), QStringLiteral("Color · zone %1"));
    m_dict.insert(QStringLiteral("Cor"), QStringLiteral("Color"));
    m_dict.insert(QStringLiteral("Desligar a retroiluminação após 30 s sem uso"),
                  QStringLiteral("Turn off backlight after 30 s of inactivity"));
    m_dict.insert(QStringLiteral("O firmware apaga o teclado depois de 30 segundos de inatividade."),
                  QStringLiteral("Firmware turns off the keyboard after 30 seconds of inactivity."));

    // Monitoring (MonitorPage)
    m_dict.insert(QStringLiteral("Temperaturas"), QStringLiteral("Temperatures"));
    m_dict.insert(QStringLiteral("Utilização"), QStringLiteral("Utilization"));
    m_dict.insert(QStringLiteral("Detalhes"), QStringLiteral("Details"));
    m_dict.insert(QStringLiteral("CPU frequência"), QStringLiteral("CPU frequency"));
    m_dict.insert(QStringLiteral("CPU uso"), QStringLiteral("CPU usage"));
    m_dict.insert(QStringLiteral("Memória"), QStringLiteral("Memory"));
    m_dict.insert(QStringLiteral("em repouso"), QStringLiteral("idle"));
    m_dict.insert(QStringLiteral("GPU núcleo / memória"), QStringLiteral("GPU core / memory"));
    m_dict.insert(QStringLiteral("GPU potência"), QStringLiteral("GPU power"));
    m_dict.insert(QStringLiteral("Temperaturas e rotações vêm do EC (GetGamingSysInfo), como no PredatorSense. Enquanto a GPU dedicada estiver em repouso não é acordada só para ler números."),
                  QStringLiteral("Temperatures and fan speeds come from EC (GetGamingSysInfo), like in PredatorSense. While the dedicated GPU is idle it is not woken up just to read numbers."));

    // Settings (SettingsPage)
    m_dict.insert(QStringLiteral("Bateria e carregamento"), QStringLiteral("Battery and charging"));
    m_dict.insert(QStringLiteral("Limite de carga a 80%"), QStringLiteral("80% charge limit"));
    m_dict.insert(QStringLiteral("Prolonga a vida útil da bateria quando o portátil fica muito tempo ligado à corrente (Battery Health Mode)."),
                  QStringLiteral("Extends battery lifespan when the laptop is plugged in for long periods (Battery Health Mode)."));
    m_dict.insert(QStringLiteral("Calibração da bateria"), QStringLiteral("Battery calibration"));
    m_dict.insert(QStringLiteral("Calibração em curso: mantenha o carregador ligado. O processo descarrega e carrega a bateria por completo e pode demorar várias horas."),
                  QStringLiteral("Calibration in progress: keep charger connected. The process completely discharges and recharges the battery and can take several hours."));
    m_dict.insert(QStringLiteral("Recalibra o indicador de carga. Requer o carregador ligado durante todo o processo."),
                  QStringLiteral("Recalibrates the battery gauge. Requires charger connected during the entire process."));
    m_dict.insert(QStringLiteral("Carregamento USB com o portátil desligado"),
                  QStringLiteral("USB charging while powered off"));
    m_dict.insert(QStringLiteral("Mantém a porta USB de carregamento ativa com o sistema desligado."),
                  QStringLiteral("Keeps the charging USB port active with the system turned off."));
    m_dict.insert(QStringLiteral("Parar o carregamento USB quando a bateria descer abaixo de"),
                  QStringLiteral("Stop USB charging when battery falls below"));
    m_dict.insert(QStringLiteral("Sistema"), QStringLiteral("System"));
    m_dict.insert(QStringLiteral("Animação e som de arranque"), QStringLiteral("Startup animation and sound"));
    m_dict.insert(QStringLiteral("Mostra a animação Predator com som ao ligar o portátil."),
                  QStringLiteral("Shows Predator animation with sound when turning on the laptop."));
    m_dict.insert(QStringLiteral("LCD Overdrive"), QStringLiteral("LCD Overdrive"));
    m_dict.insert(QStringLiteral("Reduz o tempo de resposta do ecrã."), QStringLiteral("Reduces screen response time."));
    m_dict.insert(QStringLiteral("Desligar a retroiluminação do teclado após 30 s"),
                  QStringLiteral("Turn off keyboard backlight after 30 s"));
    m_dict.insert(QStringLiteral("Overclock da GPU por modo (perfil Acer)"),
                  QStringLiteral("GPU overclock per mode (Acer profile)"));
    m_dict.insert(QStringLiteral("Valores do PredatorSense para esta GPU — Turbo: +%1 MHz núcleo, +%2 MHz memória. Aplicado via NVML."),
                  QStringLiteral("PredatorSense values for this GPU — Turbo: +%1 MHz core, +%2 MHz memory. Applied via NVML."));
    m_dict.insert(QStringLiteral("Aplicação"), QStringLiteral("Application"));
    m_dict.insert(QStringLiteral("Fechar para a bandeja do sistema"), QStringLiteral("Close to system tray"));
    m_dict.insert(QStringLiteral("A janela fecha mas o ícone continua na bandeja para trocar de modo rapidamente."),
                  QStringLiteral("Window closes but icon remains in system tray for quick mode switching."));
    m_dict.insert(QStringLiteral("Temperatura em Fahrenheit"), QStringLiteral("Temperature in Fahrenheit"));
    m_dict.insert(QStringLiteral("Idioma"), QStringLiteral("Language"));
    m_dict.insert(QStringLiteral("Selecione o idioma da aplicação."), QStringLiteral("Select application language."));
    m_dict.insert(QStringLiteral("Sobre"), QStringLiteral("About"));
    m_dict.insert(QStringLiteral("Modelo: %1\nGPU: %2\nVentoinhas: %3 CPU, %4 GPU · Teclado: %5 zonas\n\nPredator Sense para Linux reimplementa o protocolo do PredatorSense 5.0.1463 da Acer (engenharia reversa para interoperabilidade). Projeto independente, sem afiliação à Acer."),
                  QStringLiteral("Model: %1\nGPU: %2\nFans: %3 CPU, %4 GPU · Keyboard: %5 zones\n\nPredator Sense for Linux reimplements Acer's PredatorSense 5.0.1463 protocol (reverse engineering for interoperability). Independent project, not affiliated with Acer."));
}

QString AppTranslator::translate(const char *context, const char *sourceText,
                                 const char *disambiguation, int n) const
{
    Q_UNUSED(context);
    Q_UNUSED(disambiguation);
    Q_UNUSED(n);

    if (!m_active || !sourceText)
        return QString();

    const QString key = QString::fromUtf8(sourceText);
    auto it = m_dict.find(key);
    if (it != m_dict.end())
        return it.value();

    return QString();
}
