#include "AcerHardware.h"
#include "MethodIds.h"

#include <QLoggingCategory>
#include <QtEndian>

Q_LOGGING_CATEGORY(lcHw, "predator.hw")

using W = WmiDevice;

static inline bool ok(uint64_t v) { return (v & 0xFF) == 0; }

std::optional<uint64_t> AcerHardware::gaming32(unsigned method, uint32_t in)
{
    auto r = m_wmi.callU32(W::Gaming, method, in);
    if (!r)
        qCWarning(lcHw) << m_wmi.lastError();
    else
        qCDebug(lcHw, "gaming m%u in=0x%X out=0x%llX", method, in, (unsigned long long)*r);
    return r;
}

bool AcerHardware::gamingSet64(unsigned method, uint64_t in)
{
    auto r = m_wmi.callU64(W::Gaming, method, in);
    if (!r) {
        qCWarning(lcHw) << m_wmi.lastError();
        return false;
    }
    qCDebug(lcHw, "gaming m%u in=0x%llX out=0x%llX", method, (unsigned long long)in, (unsigned long long)*r);
    return ok(*r);
}

// ---------------------------------------------------------------- operating mode

std::optional<int> AcerHardware::operatingMode()
{
    // GetOperatingMode: GetGamingMiscSetting(0x0B), value in byte 1.
    auto r = gaming32(Mid::GetGamingMiscSetting, 0x0B);
    if (!r || !ok(*r))
        return std::nullopt;
    return int((*r >> 8) & 0xFF);
}

bool AcerHardware::setOperatingMode(int mode)
{
    // set_operation_mode: SetGamingMiscSetting((mode << 8) | 0x0B)
    return gamingSet64(Mid::SetGamingMiscSetting, (uint64_t(mode) << 8) | 0x0B);
}

QList<int> AcerHardware::supportedModes()
{
    // SUPPORT_OPERATING_MODE_CAPABILITY: GetGamingMiscSetting(0x0A), byte 1 is a
    // bitmap where bit n means mode n is available. Fallback used by the service
    // when the query fails: Quiet, Balanced, Performance, Turbo, Eco.
    auto r = gaming32(Mid::GetGamingMiscSetting, 0x0A);
    if (!r || !ok(*r))
        return {Quiet, Balanced, Performance, Turbo, Eco};
    const unsigned bits = (*r >> 8) & 0xFF;
    QList<int> modes;
    for (int i = 0; i < 7; ++i)
        if (bits & (1u << i))
            modes << i;
    return modes;
}

// ---------------------------------------------------------------- sensors & fans

std::optional<uint16_t> AcerHardware::supportedSensors()
{
    // FUN_14003ceb0: GetGamingSysInfo(0), bitmap in bits 24..39 (bit k = sensor k+1)
    auto r = gaming32(Mid::GetGamingSysInfo, 0);
    if (!r || !ok(*r))
        return std::nullopt;
    return uint16_t(*r >> 24);
}

std::optional<int> AcerHardware::readSensor(int sensorId)
{
    // PSSocket::Get_*: GetGamingSysInfo((id << 8) | 1), value = (out >> 8) & 0xFFFF
    auto r = m_wmi.callU32(W::Gaming, Mid::GetGamingSysInfo, (uint32_t(sensorId) << 8) | 1);
    if (!r || !ok(*r))
        return std::nullopt;
    return int((*r >> 8) & 0xFFFF);
}

AcerHardware::FanCount AcerHardware::fanCount()
{
    // PSSocket::Get_Fan_Amount (when the model INI has no [FanSupport])
    FanCount f;
    auto bits = supportedSensors();
    if (!bits)
        return f;
    if (*bits & 0x002) f.cpu = 1;
    if (*bits & 0x020) f.gpu++;
    if (*bits & 0x100) f.gpu++;
    if (*bits & 0x008) f.sys++;
    if (*bits & 0x080) f.sys++;
    return f;
}

bool AcerHardware::setFanMode(FanMode mode, const FanCount &fans)
{
    // SET_DEVICE_DATA FAN_CONTROL (Auto/Max): one SetGamingFanBehavior call.
    // low 16 bits: fan bitmap (bit 0 CPU, bit 3+n GPU n)
    // high bits:   per-fan behaviour (mode+1) at 2-bit slots, CPU at 0, GPU n at 6+2n
    const uint64_t value = uint64_t(mode) + 1; // 1 auto, 2 max
    uint64_t bitmap = 0, behaviour = 0;
    if (fans.cpu > 0) {
        bitmap |= 1;
        behaviour |= value;
    }
    for (int n = 0; n < fans.gpu; ++n) {
        bitmap |= 1ull << (3 + n);
        behaviour |= value << (6 + 2 * n);
    }
    if (!bitmap)
        return false;
    return gamingSet64(Mid::SetGamingFanBehavior, (behaviour << 16) | bitmap);
}

bool AcerHardware::setFanCustom(const FanSetting &cpu, const QList<FanSetting> &gpu, const FanCount &fans)
{
    // FUN_14003d410: fans flagged "auto" go to one SetGamingFanBehavior call with
    // behaviour 1, the others to a second call with behaviour 3 (custom); then
    // SetGamingFanSpeed((percent << 8) | fanId) for each custom fan.
    uint64_t autoMap = 0, autoBeh = 0, custMap = 0, custBeh = 0;
    if (fans.cpu > 0) {
        if (cpu.autoMode) { autoMap |= 1; autoBeh |= 1; }
        else { custMap |= 1; custBeh |= 3; }
    }
    for (int n = 0; n < fans.gpu; ++n) {
        const bool isAuto = n < gpu.size() ? gpu[n].autoMode : true;
        if (isAuto) { autoMap |= 1ull << (3 + n); autoBeh |= 1ull << (6 + 2 * n); }
        else { custMap |= 1ull << (3 + n); custBeh |= 3ull << (6 + 2 * n); }
    }
    if (autoMap && !gamingSet64(Mid::SetGamingFanBehavior, (autoBeh << 16) | autoMap))
        return false;
    if (!custMap)
        return true;
    if (!gamingSet64(Mid::SetGamingFanBehavior, (custBeh << 16) | custMap))
        return false;
    bool good = true;
    if (fans.cpu > 0 && !cpu.autoMode)
        good &= gamingSet64(Mid::SetGamingFanSpeed, (uint64_t(qBound(0, cpu.speed, 100)) << 8) | FanIdCpu);
    for (int n = 0; n < fans.gpu && n < gpu.size(); ++n)
        if (!gpu[n].autoMode)
            good &= gamingSet64(Mid::SetGamingFanSpeed,
                                (uint64_t(qBound(0, gpu[n].speed, 100)) << 8) | uint64_t(FanIdGpuBase + n));
    return good;
}

std::optional<int> AcerHardware::fanBehaviorOf(int index, bool gpu)
{
    // GET FAN_CONTROL: GetGamingFanBehavior(bitmap); the behaviour slots are
    // returned shifted by the status byte (CPU at bits 8..9).
    const uint32_t bitmap = gpu ? (1u << (3 + index)) : 1u;
    auto r = gaming32(Mid::GetGamingFanBehavior, bitmap);
    if (!r || !ok(*r))
        return std::nullopt;
    const int shift = gpu ? 8 + 6 + 2 * index : 8;
    return int((*r >> shift) & 0x3);
}

std::optional<int> AcerHardware::fanSpeedSetting(int fanId)
{
    // GET FAN_CONTROL: GetGamingFanSpeed(fanId) -> percent in byte 1
    auto r = gaming32(Mid::GetGamingFanSpeed, uint32_t(fanId));
    if (!r || !ok(*r))
        return std::nullopt;
    return int((*r >> 8) & 0xFF);
}

// ---------------------------------------------------------------- keyboard lighting

bool AcerHardware::setKbBacklight(const KbConfig &c)
{
    // ECKeyboard::SetKeyboardBacklightEffect / AcerECKeyboardController::SetMode
    QByteArray b(16, '\0');
    b[0] = char(c.effect);
    b[1] = char(c.effect == 0 ? 0 : qBound(1, c.speed, 9));
    b[2] = char(qBound(0, c.brightness, 100));
    b[3] = 0;
    b[4] = char(c.effect == 0 ? 0 : c.direction);
    b[5] = char(c.color.red());
    b[6] = char(c.color.green());
    b[7] = char(c.color.blue());
    b[8] = 3;
    b[9] = 1; // device: keyboard
    auto r = m_wmi.call(W::Gaming, Mid::SetGamingKBBacklight, b);
    if (!r) {
        qCWarning(lcHw) << m_wmi.lastError();
        return false;
    }
    return ok(r->u64());
}

bool AcerHardware::setKbZoneColors(int brightness, const QList<QColor> &zones, int ledBehaviorSize)
{
    // ECKeyboard::SetKeyboardLEDBehavior (AcerLightingService 0x14000b3c0), keyboard device:
    // 1. SetGamingLED: [0] = 8 (keyboard), [5] = mask of zones whose colour is not black.
    //    Buffer size follows the SMBIOS type 172 version (16 / 12 bytes); without the
    //    table the service sends 0x00000F0000000008 (all four zones).
    // 2. Static effect at the requested brightness (SetGamingKBBacklight).
    // 3. SetGamingRgbKb((B<<24)|(G<<16)|(R<<8)|zoneBit) for each zone.
    QByteArray led(ledBehaviorSize, '\0');
    led[0] = 8;
    uint8_t mask = 0x0F;
    if (ledBehaviorSize > 8) {
        mask = 0;
        for (int i = 0; i < zones.size() && i < 4; ++i)
            if (zones[i].rgb() & 0xFFFFFF)
                mask |= uint8_t(1u << i);
    }
    led[5] = char(mask);
    if (auto r = m_wmi.call(W::Gaming, Mid::SetGamingLED, led); !r || !ok(r->u64())) {
        qCWarning(lcHw) << "SetGamingLED (zone mask) failed" << m_wmi.lastError();
        return false;
    }

    KbConfig base;
    base.effect = 0;
    base.brightness = brightness;
    base.color = QColor(0, 0, 0);
    if (!setKbBacklight(base))
        return false;
    bool good = true;
    for (int i = 0; i < zones.size() && i < 8; ++i) {
        const QColor &c = zones[i];
        const uint64_t v = (uint64_t(c.blue()) << 24) | (uint64_t(c.green()) << 16) |
                           (uint64_t(c.red()) << 8) | (1ull << i);
        good &= gamingSet64(Mid::SetGamingRgbKb, v);
    }
    return good;
}

std::optional<AcerHardware::KbConfig> AcerHardware::kbBacklight()
{
    // MOF: GetGamingKBBacklight([in] uint32 gmInput, [out] uint8 gmReturn, [out] uint8 gmOutput[15]).
    // The returned buffer is gmReturn followed by gmOutput, which uses the setter's layout
    // (ECKeyboard reads gmReturn into [0] and gmOutput[0..9] into [1..10]).
    auto r = m_wmi.call(W::Gaming, Mid::GetGamingKBBacklight, QByteArray("\x01\x00\x00\x00", 4));
    if (!r || r->data.size() < 9 || quint8(r->data[0]) != 0)
        return std::nullopt;
    const QByteArray d = r->data.mid(1);
    KbConfig c;
    c.effect = quint8(d[0]);
    c.speed = quint8(d[1]);
    c.brightness = quint8(d[2]);
    c.direction = quint8(d[4]);
    c.color = QColor(quint8(d[5]), quint8(d[6]), quint8(d[7]));
    return c;
}

std::optional<bool> AcerHardware::kbTimeout(int bk)
{
    // BACKLIGHT_30S get: GetFunction((bk << 8) | 0x80001); byte 5 == 30 -> enabled
    auto r = m_wmi.callU32(W::APGe, Mid::GetFunction, (uint32_t(bk) << 8) | 0x80001);
    if (!r || !ok(*r))
        return std::nullopt;
    return ((*r >> 40) & 0xFF) == 0x1E;
}

bool AcerHardware::setKbTimeout(int bk, bool enabled)
{
    // FUN_14003de10: keep the current brightness (byte 4 of the get result,
    // 100 if unreadable) and write SetFunction((bk<<8) | 0x80002 | ((timeout<<8)|brightness) << 32).
    uint64_t brightness = 100;
    if (auto r = m_wmi.callU32(W::APGe, Mid::GetFunction, (uint32_t(bk) << 8) | 0x80001); r && ok(*r))
        brightness = (*r >> 32) & 0xFF;
    const uint64_t timeout = enabled ? 0x1E00 : 0;
    const uint64_t in = (uint64_t(bk) << 8) | 0x80002 | ((timeout | brightness) << 32);
    auto r = m_wmi.callU64(W::APGe, Mid::SetFunction, in);
    return r && ok(*r);
}

// ---------------------------------------------------------------- misc

std::optional<bool> AcerHardware::bootSound()
{
    auto r = gaming32(Mid::GetGamingMiscSetting, 6);
    if (!r || !ok(*r))
        return std::nullopt;
    const uint64_t v = *r & 0xFF00;
    if (v == 0x100) return true;
    if (v == 0) return false;
    return std::nullopt;
}

bool AcerHardware::setBootSound(bool on)
{
    return gamingSet64(Mid::SetGamingMiscSetting, on ? 0x106 : 0x6);
}

std::optional<int> AcerHardware::lcdOverdriveRaw()
{
    auto r = gaming32(Mid::GetGamingProfile, 0);
    if (!r || !ok(*r))
        return std::nullopt;
    return int((*r >> 48) & 0xFF);
}

bool AcerHardware::setLcdOverdrive(bool on)
{
    return gamingSet64(Mid::SetGamingProfile, on ? 0x1000000000010ull : 0x10ull);
}

std::optional<AcerHardware::UsbCharge> AcerHardware::usbCharging()
{
    // GetFunction(4): byte 1 = 0x0F enabled / 0x1F disabled, byte 2 = battery % threshold
    auto r = m_wmi.callU32(W::APGe, Mid::GetFunction, 4);
    if (!r || !ok(*r))
        return std::nullopt;
    UsbCharge u;
    u.enabled = ((*r >> 8) & 0xFF) == 0x0F;
    u.threshold = int((*r >> 16) & 0xFF);
    return u;
}

bool AcerHardware::setUsbCharging(const UsbCharge &u)
{
    // SetFunction((threshold << 16) | (state << 8) | 4)
    const uint64_t state = u.enabled ? 0x0F : 0x1F;
    const uint64_t thr = (u.threshold == 20 || u.threshold == 30) ? u.threshold : 10;
    auto r = m_wmi.callU64(W::APGe, Mid::SetFunction, (thr << 16) | (state << 8) | 4);
    return r && ok(*r);
}

std::optional<AcerHardware::BatteryHealth> AcerHardware::batteryHealth()
{
    // MOF: GetBatteryHealthControlStatus([in] uint8 uBatteryNo, uFunctionQuery, uint8 uReserved[2],
    // [out] uint8 uFunctionList, uint8 uReturn[2], uint8 uFunctionStatus[5])
    const char in[4] = {1, 1, 0, 0};
    auto r = m_wmi.call(W::Battery, Mid::GetBatteryHealthControlStatus, QByteArray(in, 4));
    if (!r || r->data.size() < 5)
        return std::nullopt;
    BatteryHealth h;
    h.healthMode = quint8(r->data[3]) != 0;
    h.calibration = quint8(r->data[4]) != 0;
    return h;
}

bool AcerHardware::setBatteryFunction(int function, bool enabled)
{
    // MOF: SetBatteryHealthControl([in] uint8 uBatteryNo, uFunctionMask, uFunctionStatus,
    // uint8 uReservedIn[5], [out] uint16 uReturn, uint16 uReservedOut)
    const char in[8] = {1, char(function), char(enabled ? 1 : 0), 0, 0, 0, 0, 0};
    auto r = m_wmi.call(W::Battery, Mid::SetBatteryHealthControl, QByteArray(in, 8));
    if (!r || r->data.size() < 2)
        return false;
    return (quint8(r->data[0]) | (quint8(r->data[1]) << 8)) == 0;
}

std::optional<int> AcerHardware::batteryBoostRaw()
{
    auto r = gaming32(Mid::GetGamingSysInfo, 2);
    if (!r || !ok(*r))
        return std::nullopt;
    return int((*r >> 40) & 0xFF);
}
