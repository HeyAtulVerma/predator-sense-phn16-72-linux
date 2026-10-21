#include "Daemon.h"

#include <QDBusConnection>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLoggingCategory>
#include <QSaveFile>
#include <QSocketNotifier>
#include <fcntl.h>
#include <unistd.h>

Q_LOGGING_CATEGORY(lcD, "predator.daemon", QtInfoMsg)

using HW = AcerHardware;

namespace {

const char *const kEffectNames[] = {"static", "breathing", "neon", "wave", "shifting", "zoom", "meteor", "twinkling"};

int effectFromName(const QString &n)
{
    for (int i = 0; i < 8; ++i)
        if (n == QLatin1String(kEffectNames[i]))
            return i;
    return -1;
}

QString modeName(int m)
{
    switch (m) {
    case HW::Quiet: return QStringLiteral("quiet");
    case HW::Balanced: return QStringLiteral("balanced");
    case HW::Performance: return QStringLiteral("performance");
    case HW::Turbo: return QStringLiteral("turbo");
    case HW::Eco: return QStringLiteral("eco");
    default: return QStringLiteral("mode%1").arg(m);
    }
}

QByteArray readFile(const QString &p)
{
    QFile f(p);
    return f.open(QIODevice::ReadOnly) ? f.readAll().trimmed() : QByteArray();
}

QJsonObject defaultFan()
{
    QJsonObject cpu{{QStringLiteral("auto"), true}, {QStringLiteral("speed"), 50}};
    QJsonArray gpu;
    gpu.append(QJsonObject{{QStringLiteral("auto"), true}, {QStringLiteral("speed"), 50}});
    gpu.append(QJsonObject{{QStringLiteral("auto"), true}, {QStringLiteral("speed"), 50}});
    return {{QStringLiteral("mode"), QStringLiteral("auto")}, {QStringLiteral("cpu"), cpu}, {QStringLiteral("gpu"), gpu}};
}

QJsonObject defaultLighting()
{
    // DEFAULT_LIGHTING_KEYBOARD of the PredatorSense UI (#00aec7, static)
    QJsonArray zones;
    for (int i = 0; i < 4; ++i)
        zones.append(QStringLiteral("#00aec7"));
    return {{QStringLiteral("managed"), false},
            {QStringLiteral("effect"), QStringLiteral("static")},
            {QStringLiteral("zones"), zones},
            {QStringLiteral("color"), QStringLiteral("#00aec7")},
            {QStringLiteral("speed"), 5},
            {QStringLiteral("brightness"), 100},
            {QStringLiteral("direction"), 1}};
}

} // namespace

Daemon::Daemon(const Options &opt, QObject *parent) : QObject(parent), m_opt(opt)
{
    m_fan = defaultFan();
    m_lighting = defaultLighting();
    m_pollTimer.setInterval(2000);
    connect(&m_pollTimer, &QTimer::timeout, this, &Daemon::poll);
}

bool Daemon::start(QString *error)
{
    if (m_opt.simulate) {
        m_wmi.enableSimulation();
    } else if (!m_wmi.open(m_opt.devicePath)) {
        *error = m_wmi.lastError() + QStringLiteral(" (is the predator_wmi module loaded?)");
        return false;
    }
    if (!m_wmi.hasIface(WmiDevice::Gaming)) {
        *error = QStringLiteral("AcerGamingFunction WMI interface not present on this machine");
        return false;
    }
    m_product = QString::fromUtf8(readFile(QStringLiteral("/sys/class/dmi/id/product_name")));
    m_oem = SmbiosOem::read();
    if (m_opt.simulate) {
        m_product = QStringLiteral("Predator PHN16-72");
        // values of the PHN16-72 BIOS V1.20 SMBIOS tables 170/172
        m_oem.keyboardArea = 4;
        m_oem.keyboardColor = 2;
        m_oem.keyboardInterface = 1;
        m_oem.tableType = 2;
        m_oem.tableVersion = 0x5B;
        m_oem.bkHotkey = 0x84;
    }
    m_model = ModelConfig::load(m_opt.modelDir, m_product);
    detectCapabilities();
    const bool hadState = loadState();
    watchPlatformProfile();

    QDBusConnection::systemBus().connect(QStringLiteral("org.freedesktop.login1"),
                                         QStringLiteral("/org/freedesktop/login1"),
                                         QStringLiteral("org.freedesktop.login1.Manager"),
                                         QStringLiteral("PrepareForSleep"), this, SLOT(onPrepareForSleep(bool)));

    if (hadState) {
        restoreAll("startup");
    } else {
        // First run: adopt what the firmware has, change nothing.
        m_lastAC = onAC();
        m_currentMode = m_hw.operatingMode().value_or(-1);
        if (m_currentMode >= 0) {
            if (m_lastAC)
                m_acMode = m_currentMode;
            else if (m_currentMode == HW::Balanced || m_currentMode == HW::Eco)
                m_dcMode = m_currentMode;
            afterModeChange(m_currentMode);
        }
        saveState();
    }
    m_pollTimer.start();
    return true;
}

void Daemon::detectCapabilities()
{
    m_modes = m_hw.supportedModes();
    m_sensorMask = m_hw.supportedSensors().value_or(0);

    // PSSocket::Get_Fan_Amount: the model INI [FanSupport] wins, then the sensor bitmap.
    m_fans = m_hw.fanCount();
    if (auto v = m_model.fanSupport(QStringLiteral("CPU"))) m_fans.cpu = *v;
    if (auto v = m_model.fanSupport(QStringLiteral("GPU"))) m_fans.gpu = *v;
    if (auto v = m_model.fanSupport(QStringLiteral("System"))) m_fans.sys = *v;

    const auto lcd = m_hw.lcdOverdriveRaw();
    m_lcdSupported = lcd && *lcd != 0xFF;
    m_bootSoundSupported = m_hw.bootSound().has_value();
    m_usbChargeSupported = m_wmi.hasIface(WmiDevice::APGe) && m_hw.usbCharging().has_value();
    m_batterySupported = m_wmi.hasIface(WmiDevice::Battery) && m_hw.batteryHealth().has_value();
    m_kbTimeoutSupported = m_oem.bkHotkey > 0 && m_oem.keyboardArea != 0 && m_wmi.hasIface(WmiDevice::APGe) &&
                           m_hw.kbTimeout(m_oem.bkHotkey).has_value();
    if (m_gpu.present())
        m_oc = m_model.gpuOc(m_gpu.pciId());

    qCInfo(lcD) << "model" << m_product << "modes" << m_modes << "fans cpu/gpu/sys" << m_fans.cpu << m_fans.gpu
                << m_fans.sys << "sensors" << Qt::hex << m_sensorMask << Qt::dec << "kb zones" << m_oem.keyboardArea
                << "rgb" << m_oem.keyboardColor << "smbios172" << Qt::hex << m_oem.tableType << m_oem.tableVersion << Qt::dec
                << "bk" << m_oem.bkHotkey << "gpu" << m_gpu.pciId()
                << "oc" << m_oc.has_value();
}

// ------------------------------------------------------------------ persistence

bool Daemon::loadState()
{
    QFile f(m_opt.statePath);
    if (!f.open(QIODevice::ReadOnly))
        return false;
    const QJsonObject o = QJsonDocument::fromJson(f.readAll()).object();
    m_fanManaged = o.value(QStringLiteral("fanManaged")).toBool(false);
    m_acMode = o.value(QStringLiteral("acMode")).toInt(m_acMode);
    m_dcMode = o.value(QStringLiteral("dcMode")).toInt(m_dcMode);
    m_lastNonTurbo = o.value(QStringLiteral("lastNonTurboMode")).toInt(m_lastNonTurbo);
    if (o.value(QStringLiteral("fan")).isObject())
        m_fan = o.value(QStringLiteral("fan")).toObject();
    if (o.value(QStringLiteral("lighting")).isObject())
        m_lighting = o.value(QStringLiteral("lighting")).toObject();
    m_gpuOcEnabled = o.value(QStringLiteral("gpuOcInModes")).toBool(true);
    return true;
}

void Daemon::saveState()
{
    QDir().mkpath(QFileInfo(m_opt.statePath).absolutePath());
    QSaveFile f(m_opt.statePath);
    if (!f.open(QIODevice::WriteOnly))
        return;
    QJsonObject o{{QStringLiteral("acMode"), m_acMode},
                  {QStringLiteral("dcMode"), m_dcMode},
                  {QStringLiteral("lastNonTurboMode"), m_lastNonTurbo},
                  {QStringLiteral("fan"), m_fan},
                  {QStringLiteral("lighting"), m_lighting},
                  {QStringLiteral("gpuOcInModes"), m_gpuOcEnabled},
                  {QStringLiteral("fanManaged"), m_fanManaged}};
    f.write(QJsonDocument(o).toJson());
    f.commit();
}

// ------------------------------------------------------------------ helpers

bool Daemon::onAC() const
{
    QDir ps(QStringLiteral("/sys/class/power_supply"));
    bool sawMains = false;
    for (const QString &d : ps.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (readFile(ps.filePath(d + QStringLiteral("/type"))) != "Mains")
            continue;
        sawMains = true;
        if (readFile(ps.filePath(d + QStringLiteral("/online"))) == "1")
            return true;
    }
    // USB-C PD adapters show up as type "USB"
    for (const QString &d : ps.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
        if (readFile(ps.filePath(d + QStringLiteral("/type"))) == "USB" &&
            readFile(ps.filePath(d + QStringLiteral("/online"))) == "1")
            return true;
    return !sawMains; // desktop-like fallback
}

bool Daemon::modeAllowedNow(int mode) const
{
    if (!m_modes.contains(mode))
        return false;
    // onBatteryPowerMode(): on battery only Balanced (DEFAULT) and Eco are offered.
    if (!onAC())
        return mode == HW::Balanced || mode == HW::Eco;
    return true;
}

bool Daemon::applyMode(int mode)
{
    if (!m_hw.setOperatingMode(mode))
        return false;
    m_currentMode = mode;
    afterModeChange(mode);
    return true;
}

void Daemon::afterModeChange(int mode)
{
    if (mode != HW::Turbo)
        m_lastNonTurbo = mode;
    applyGpuOc(mode, true);
}

void Daemon::applyGpuOc(int mode, bool force)
{
    if (!m_oc || !m_gpu.present() || !m_gpuOcEnabled)
        return;
    const int level = ModelConfig::ocLevelForMode(mode);
    const int core = m_oc->core[level];
    const int mem = m_oc->mem[level];
    if (!force && m_ocAppliedLevel == (core << 16 | mem))
        return;
    if (m_opt.simulate) {
        qCInfo(lcD) << "[simulate] would set GPU offsets core" << core << "mem" << mem;
        m_ocAppliedLevel = core << 16 | mem;
        return;
    }
    // Do not wake a sleeping dGPU: offsets are applied when it powers up (see poll()).
    if (!m_gpu.runtimeActive()) {
        m_ocAppliedLevel = -1;
        return;
    }
    if (m_gpu.setClockOffsets(core, mem)) {
        m_ocAppliedLevel = core << 16 | mem;
        qCInfo(lcD) << "GPU offsets core" << core << "MHz mem" << mem << "MHz for mode" << modeName(mode);
    }
}

bool Daemon::applyFan()
{
    const QString mode = m_fan.value(QStringLiteral("mode")).toString();
    if (mode == QLatin1String("max"))
        return m_hw.setFanMode(HW::FanMax, m_fans);
    if (mode != QLatin1String("custom"))
        return m_hw.setFanMode(HW::FanAuto, m_fans);

    auto toSetting = [](const QJsonObject &o) {
        HW::FanSetting s;
        s.autoMode = o.value(QStringLiteral("auto")).toBool(true);
        s.speed = o.value(QStringLiteral("speed")).toInt(50);
        return s;
    };
    QList<HW::FanSetting> gpu;
    for (const QJsonValue &v : m_fan.value(QStringLiteral("gpu")).toArray())
        gpu << toSetting(v.toObject());
    return m_hw.setFanCustom(toSetting(m_fan.value(QStringLiteral("cpu")).toObject()), gpu, m_fans);
}

bool Daemon::applyLighting()
{
    if (m_oem.keyboardArea == 0)
        return false; // per-key USB keyboards are not handled by the EC path
    const int effect = effectFromName(m_lighting.value(QStringLiteral("effect")).toString());
    const int brightness = m_lighting.value(QStringLiteral("brightness")).toInt(100);
    if (effect == 0) {
        QList<QColor> zones;
        for (const QJsonValue &v : m_lighting.value(QStringLiteral("zones")).toArray())
            zones << QColor(v.toString());
        const int n = m_oem.keyboardArea > 0 ? m_oem.keyboardArea : 4;
        while (zones.size() < n)
            zones << QColor(0x00, 0xae, 0xc7);
        int ledSize = 8;
        if (m_oem.tableType == 2 && m_oem.tableVersion >= 0x5B)
            ledSize = 16;
        else if (m_oem.tableType == 2 && m_oem.tableVersion >= 0x56)
            ledSize = 12;
        return m_hw.setKbZoneColors(brightness, zones.mid(0, n), ledSize);
    }
    if (effect < 0)
        return false;
    HW::KbConfig c;
    c.effect = effect;
    c.speed = m_lighting.value(QStringLiteral("speed")).toInt(5);
    c.brightness = brightness;
    c.direction = m_lighting.value(QStringLiteral("direction")).toInt(1);
    c.color = QColor(m_lighting.value(QStringLiteral("color")).toString());
    return m_hw.setKbBacklight(c);
}

void Daemon::restoreAll(const char *reason)
{
    qCInfo(lcD) << "restoring settings:" << reason;
    const bool ac = onAC();
    m_lastAC = ac;
    int want = ac ? m_acMode : m_dcMode;
    if (!modeAllowedNow(want))
        want = HW::Balanced;
    applyMode(want);
    if (m_fanManaged)
        applyFan();
    if (m_lighting.value(QStringLiteral("managed")).toBool())
        applyLighting();
    Q_EMIT stateChanged();
}

void Daemon::watchPlatformProfile()
{
    // With acer_wmi loaded the mode hotkey changes /sys/firmware/acpi/platform_profile,
    // which supports sysfs_notify(): react at once instead of polling the EC.
    m_profileFd = ::open("/sys/firmware/acpi/platform_profile", O_RDONLY | O_CLOEXEC);
    if (m_profileFd < 0 || m_opt.simulate)
        return;
    char buf[64];
    if (::read(m_profileFd, buf, sizeof buf) < 0) { // arms the notification
        ::close(m_profileFd);
        m_profileFd = -1;
        return;
    }
    m_profileNotifier = new QSocketNotifier(m_profileFd, QSocketNotifier::Exception, this);
    connect(m_profileNotifier, &QSocketNotifier::activated, this, [this] {
        char b[64];
        if (::lseek(m_profileFd, 0, SEEK_SET) < 0 || ::read(m_profileFd, b, sizeof b) < 0)
            return;
        if (checkMode())
            Q_EMIT stateChanged();
    });
}

// ------------------------------------------------------------------ periodic work

void Daemon::poll()
{
    bool changed = false;

    // Adaptor plug/unplug: PredatorSense "AdaptorChange" lock (sysfs only, no EC access).
    const bool ac = onAC();
    if (int(ac) != m_lastAC) {
        m_lastAC = ac;
        int want = ac ? m_acMode : m_dcMode;
        if (!modeAllowedNow(want))
            want = HW::Balanced;
        qCInfo(lcD) << (ac ? "AC connected" : "on battery") << "-> mode" << modeName(want);
        applyMode(want);
        changed = true;
    }

    // Ask the EC for the mode every 2 s, or every 10 s when platform_profile notifies us.
    if (!m_profileNotifier || ++m_pollTick % 5 == 0)
        changed |= checkMode();

    // dGPU power transitions: offsets are lost when the GPU powers down.
    const bool gpuActive = m_gpu.runtimeActive();
    if (gpuActive && !m_gpuWasActive && m_currentMode >= 0)
        applyGpuOc(m_currentMode, true);
    if (!gpuActive)
        m_ocAppliedLevel = -1;
    m_gpuWasActive = gpuActive;

    if (changed)
        Q_EMIT stateChanged();
}

bool Daemon::checkMode()
{
    // Mode changed outside the daemon (mode hotkey handled by acer-wmi, power-profiles-daemon, EC, ...)
    auto m = m_hw.operatingMode();
    if (!m || *m == m_currentMode)
        return false;
    const bool ac = onAC();
    qCInfo(lcD) << "operating mode changed externally to" << modeName(*m);
    if (!ac && *m != HW::Balanced && *m != HW::Eco) {
        // PredatorSense keeps the battery lock: only Balanced/Eco on DC.
        qCInfo(lcD) << "not allowed on battery, back to" << modeName(m_dcMode);
        applyMode(modeAllowedNow(m_dcMode) ? m_dcMode : int(HW::Balanced));
    } else {
        m_currentMode = *m;
        if (ac)
            m_acMode = *m;
        else
            m_dcMode = *m;
        afterModeChange(*m);
        saveState();
    }
    return true;
}

void Daemon::onPrepareForSleep(bool sleeping)
{
    if (sleeping)
        return;
    // PSSdk.ServiceResume -> handleUpdateAfterResumed: give the EC a moment.
    QTimer::singleShot(2000, this, [this] { restoreAll("resume"); });
}

// ------------------------------------------------------------------ public API

QJsonObject Daemon::info() const
{
    QJsonArray modes;
    for (int m : m_modes)
        modes.append(QJsonObject{{QStringLiteral("value"), m}, {QStringLiteral("name"), modeName(m)}});
    QJsonArray effects;
    for (const char *e : kEffectNames)
        effects.append(QString::fromLatin1(e));
    QJsonObject oc;
    if (m_oc) {
        QJsonArray core, mem;
        for (int i = 0; i < 4; ++i) { core.append(m_oc->core[i]); mem.append(m_oc->mem[i]); }
        oc = {{QStringLiteral("core"), core}, {QStringLiteral("mem"), mem}};
    }
    return {
        {QStringLiteral("product"), m_product},
        {QStringLiteral("version"), QStringLiteral(PREDATOR_VERSION)},
        {QStringLiteral("modes"), modes},
        {QStringLiteral("batteryModes"), QJsonArray{HW::Balanced, HW::Eco}},
        {QStringLiteral("fans"), QJsonObject{{QStringLiteral("cpu"), m_fans.cpu}, {QStringLiteral("gpu"), m_fans.gpu},
                                             {QStringLiteral("sys"), m_fans.sys}}},
        {QStringLiteral("sensorMask"), int(m_sensorMask)},
        {QStringLiteral("keyboard"), QJsonObject{{QStringLiteral("zones"), m_oem.keyboardArea},
                                                 {QStringLiteral("color"), m_oem.keyboardColor},
                                                 {QStringLiteral("effects"), effects},
                                                 {QStringLiteral("timeout"), m_kbTimeoutSupported}}},
        {QStringLiteral("lcdOverdrive"), m_lcdSupported},
        {QStringLiteral("bootSound"), m_bootSoundSupported},
        {QStringLiteral("usbCharging"), m_usbChargeSupported},
        {QStringLiteral("battery"), m_batterySupported},
        {QStringLiteral("gpu"), QJsonObject{{QStringLiteral("present"), m_gpu.present()},
                                            {QStringLiteral("pciId"), m_gpu.pciId()},
                                            {QStringLiteral("oc"), oc}}},
    };
}

QJsonObject Daemon::state()
{
    QJsonObject s{
        {QStringLiteral("mode"), m_currentMode},
        {QStringLiteral("acMode"), m_acMode},
        {QStringLiteral("dcMode"), m_dcMode},
        {QStringLiteral("onAC"), onAC()},
        {QStringLiteral("fan"), m_fan},
        {QStringLiteral("lighting"), m_lighting},
        {QStringLiteral("gpuOcInModes"), m_gpuOcEnabled},
    };
    // Firmware-held settings are read back from the hardware every time.
    if (m_bootSoundSupported)
        if (auto v = m_hw.bootSound()) s.insert(QStringLiteral("bootSound"), *v);
    if (m_lcdSupported)
        if (auto v = m_hw.lcdOverdriveRaw()) s.insert(QStringLiteral("lcdOverdrive"), *v == 1);
    if (m_kbTimeoutSupported)
        if (auto v = m_hw.kbTimeout(m_oem.bkHotkey)) s.insert(QStringLiteral("kbTimeout"), *v);
    if (m_usbChargeSupported)
        if (auto v = m_hw.usbCharging()) {
            s.insert(QStringLiteral("usbCharging"), v->enabled);
            s.insert(QStringLiteral("usbChargingThreshold"), v->threshold);
        }
    if (m_batterySupported)
        if (auto v = m_hw.batteryHealth()) {
            s.insert(QStringLiteral("batteryLimit"), v->healthMode);
            s.insert(QStringLiteral("batteryCalibration"), v->calibration);
        }
    return s;
}

QJsonObject Daemon::sensors()
{
    if (m_sensorAge.isValid() && m_sensorAge.elapsed() < 800)
        return m_sensorCache;
    QJsonObject o;
    auto put = [&](const char *key, int sensorBit, int sensorId) {
        if (!(m_sensorMask & (1u << sensorBit)))
            return;
        if (auto v = m_hw.readSensor(sensorId))
            o.insert(QLatin1String(key), *v);
    };
    // bit k of the supported-sensor bitmap = sensor id k+1
    put("cpuTemp", 0, HW::SensorCpuTemp);
    put("cpuFan", 1, HW::SensorCpuFan);
    put("sysTemp", 2, HW::SensorSysTemp);
    put("sys1Fan", 3, HW::SensorSys1Fan);
    put("gpu1Fan", 5, HW::SensorGpu1Fan);
    put("sys2Fan", 7, HW::SensorSys2Fan);
    put("gpu2Fan", 8, HW::SensorGpu2Fan);
    put("gpu1Temp", 9, HW::SensorGpu1Temp);
    put("gpu2Temp", 10, HW::SensorGpu2Temp);

    const SystemStats::Snapshot st = m_stats.sample();
    o.insert(QStringLiteral("cpuUsage"), st.cpuUsage);
    o.insert(QStringLiteral("cpuFreq"), st.cpuFreqMHz);
    o.insert(QStringLiteral("cpuMaxFreq"), st.cpuMaxFreqMHz);
    o.insert(QStringLiteral("ramUsage"), st.ramUsage);
    o.insert(QStringLiteral("ramTotal"), st.ramTotalGB);

    const NvidiaGpu::Stats g = m_gpu.stats();
    o.insert(QStringLiteral("gpuIdle"), g.idle);
    if (!g.idle) {
        o.insert(QStringLiteral("gpuUsage"), g.usage);
        o.insert(QStringLiteral("gpuCoreClock"), g.coreMHz);
        o.insert(QStringLiteral("gpuMemClock"), g.memMHz);
        o.insert(QStringLiteral("gpuPower"), g.powerW);
    }
    m_sensorCache = o;
    m_sensorAge.start();
    return o;
}

QString Daemon::setOperatingMode(int mode)
{
    if (!m_modes.contains(mode))
        return QStringLiteral("mode not supported");
    if (!modeAllowedNow(mode))
        return QStringLiteral("only Balanced or Eco are available on battery");
    const int previous = m_currentMode;
    if (!applyMode(mode))
        return QStringLiteral("firmware rejected the mode");
    if (onAC())
        m_acMode = mode;
    else
        m_dcMode = mode;
    // setOperatingMode() in the PredatorSense UI: entering or leaving Quiet resets fans to Auto.
    if ((mode == HW::Quiet || previous == HW::Quiet) &&
        m_fan.value(QStringLiteral("mode")).toString() != QLatin1String("auto")) {
        m_fan.insert(QStringLiteral("mode"), QStringLiteral("auto"));
        applyFan();
    }
    saveState();
    Q_EMIT stateChanged();
    return {};
}

QString Daemon::setBatteryMode(int mode)
{
    if (mode != HW::Balanced && mode != HW::Eco)
        return QStringLiteral("battery mode must be Balanced or Eco");
    if (!m_modes.contains(mode))
        return QStringLiteral("mode not supported");
    m_dcMode = mode;
    if (!onAC())
        applyMode(mode);
    saveState();
    Q_EMIT stateChanged();
    return {};
}

QString Daemon::setFanControl(const QJsonObject &fan)
{
    const QString mode = fan.value(QStringLiteral("mode")).toString();
    if (mode != QLatin1String("auto") && mode != QLatin1String("max") && mode != QLatin1String("custom"))
        return QStringLiteral("fan mode must be auto, max or custom");
    if (m_currentMode == HW::Quiet && mode != QLatin1String("auto"))
        return QStringLiteral("fan control is locked to Auto in Quiet mode");
    QJsonObject merged = m_fan;
    for (auto it = fan.begin(); it != fan.end(); ++it)
        merged.insert(it.key(), it.value());
    const QJsonObject previous = m_fan;
    m_fan = merged;
    if (!applyFan()) {
        m_fan = previous;
        return QStringLiteral("firmware rejected the fan setting");
    }
    m_fanManaged = true;
    saveState();
    Q_EMIT stateChanged();
    return {};
}

QString Daemon::setKeyboardLighting(const QJsonObject &l)
{
    if (m_oem.keyboardArea == 0)
        return QStringLiteral("per-key USB keyboards are not supported yet");
    QJsonObject merged = m_lighting;
    for (auto it = l.begin(); it != l.end(); ++it)
        merged.insert(it.key(), it.value());
    if (effectFromName(merged.value(QStringLiteral("effect")).toString()) < 0)
        return QStringLiteral("unknown effect");
    merged.insert(QStringLiteral("managed"), true);
    const QJsonObject previous = m_lighting;
    m_lighting = merged;
    if (!applyLighting()) {
        m_lighting = previous;
        return QStringLiteral("firmware rejected the lighting setting");
    }
    saveState();
    Q_EMIT stateChanged();
    return {};
}

QString Daemon::setOption(const QString &key, const QVariant &value)
{
    bool ok = false;
    if (key == QLatin1String("bootSound") && m_bootSoundSupported) {
        ok = m_hw.setBootSound(value.toBool());
    } else if (key == QLatin1String("lcdOverdrive") && m_lcdSupported) {
        ok = m_hw.setLcdOverdrive(value.toBool());
    } else if (key == QLatin1String("kbTimeout") && m_kbTimeoutSupported) {
        ok = m_hw.setKbTimeout(m_oem.bkHotkey, value.toBool());
    } else if ((key == QLatin1String("usbCharging") || key == QLatin1String("usbChargingThreshold")) &&
               m_usbChargeSupported) {
        // USBChargeSwitch and USBCharge are separate in AcerQAAgent: each keeps the other's value.
        auto cur = m_hw.usbCharging();
        if (cur) {
            if (key == QLatin1String("usbCharging"))
                cur->enabled = value.toBool();
            else {
                const int t = value.toInt();
                if (t != 10 && t != 20 && t != 30)
                    return QStringLiteral("threshold must be 10, 20 or 30");
                cur->threshold = t;
            }
            ok = m_hw.setUsbCharging(*cur);
        }
    } else if (key == QLatin1String("batteryLimit") && m_batterySupported) {
        ok = m_hw.setBatteryFunction(1, value.toBool());
    } else if (key == QLatin1String("batteryCalibration") && m_batterySupported) {
        if (value.toBool() && !onAC())
            return QStringLiteral("connect the AC adapter before starting calibration");
        ok = m_hw.setBatteryFunction(2, value.toBool());
    } else if (key == QLatin1String("gpuOcInModes")) {
        m_gpuOcEnabled = value.toBool();
        if (m_gpuOcEnabled && m_currentMode >= 0)
            applyGpuOc(m_currentMode, true);
        else if (!m_gpuOcEnabled && m_oc && !m_opt.simulate && m_gpu.runtimeActive())
            m_gpu.setClockOffsets(0, 0); // undo what we applied, then leave the clocks alone
        m_ocAppliedLevel = -1;
        saveState();
        ok = true;
    } else {
        return QStringLiteral("unknown or unsupported option: ") + key;
    }
    if (!ok)
        return QStringLiteral("firmware rejected the setting");
    Q_EMIT stateChanged();
    return {};
}
