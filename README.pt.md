<div align="center">

<img src="data/predator-sense.svg" width="96" alt="">

# Predator Sense para Linux

**Reimplementação nativa do Acer PredatorSense para Linux, feita por engenharia reversa do software original.**

[English](README.md)

<img src="docs/screenshots/home.png" width="820" alt="Página inicial">

</div>

> [!IMPORTANT]
> Projeto não oficial, sem afiliação à Acer. "Predator" e "PredatorSense" são marcas da Acer Inc.
> Altera definições do firmware (ventoinhas, limites de energia, carregamento) usando as mesmas chamadas do software da Acer. Use por sua conta e risco.

## O que é

O PredatorSense 5.0.1463 foi decompilado (Ghidra) e cada valor enviado ao firmware foi rastreado até o código da Acer
(interface Electron, `AcerAgentService`, `AcerHardwareService`, `AcerLightingService`, agentes QuickAccess/CareCenter).
O resultado reproduz o comportamento original — inclusive as regras que outras ferramentas não seguem:

- ventoinhas agrupadas como a Acer faz (grupos automático/personalizado, IDs por ventoinha, quantidade lida do firmware);
- o desligar da retroiluminação após 30 s mantém o brilho atual;
- carregamento USB com o portátil desligado: interruptor independente do limiar 10/20/30 %;
- na bateria só Equilibrado ou Eco, com modo memorizado separado para tomada e bateria;
- entrar ou sair do Silencioso volta as ventoinhas para Automático;
- Turbo aplica o perfil de clock de GPU da Acer para a sua GPU (PHN16-72 + RTX 4060: +100 MHz núcleo / +200 MHz memória).

Protocolo completo em [docs/REVERSE_ENGINEERING.md](docs/REVERSE_ENGINEERING.md).

## Funções

Modos de funcionamento · controlo das ventoinhas (Auto/Máx/Personalizado) · teclado RGB de 4 zonas com 8 efeitos ·
limite de carga 80 % · calibração da bateria · carregamento USB desligado · animação/som de arranque · LCD overdrive ·
monitorização (temperaturas e RPM do EC, CPU/RAM/GPU) · ícone na bandeja · tecla de modo · restauro após suspensão · CLI `predatorctl`.

## Instalação (Arch / CachyOS)

```bash
git clone https://github.com/aishend/predator-sense-phn16-72-linux.git
cd predator-sense-phn16-72-linux/packaging/arch
makepkg -si
```

O driver `acer_wmi` do kernel trata a tecla de modo e as teclas de atalho; confirme que não está bloqueado
(`grep -r acer_wmi /etc/modprobe.d/`).

O módulo do kernel é instalado via DKMS e recompilado automaticamente a cada kernel novo (incluindo kernels
compilados com clang, como os do CachyOS). Tenha os headers do seu kernel instalados.

## Uso

```bash
predatorctl status
predatorctl mode turbo
predatorctl fan custom 70 60
predatorctl kb static '#ff0000' '#ffb000' '#00aec7' '#8a2be2'
predatorctl set batteryLimit on
```

Mais detalhes (arquitetura, segurança, desenvolvimento sem hardware) no [README em inglês](README.md).

## Licença

GPL-2.0-or-later. Nenhum código ou binário da Acer está incluído.
