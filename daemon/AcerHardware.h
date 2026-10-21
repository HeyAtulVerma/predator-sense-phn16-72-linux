#pragma once

#include "WmiDevice.h"

#include <QColor>
#include <QList>
#include <array>
#include <optional>

// Re-implementation of what AcerAgentService/AcerHardwareService/
// AcerLightingService/AcerQAAgent send to the firmware. Every encoding here
// comes from those binaries (docs/REVERSE_ENGINEERING.md).
//
// Convention of AcerGamingFunction integer results: byte 0 is a status code
// (0 = success) and the payload starts at bit 8.
class AcerHardware
{
public:
    // Values of GetGamingMiscSetting(0x0B) / UI OPERATING_MODE enum.
    enum Mode { Quiet = 0, Balanced = 1, Performance = 4, Turbo = 5, Eco = 6 };
    enum FanMode { FanAuto = 0, FanMax = 1, FanCustom = 2 };

    // Sensor IDs of GetGamingSysInfo((id << 8) | 1), from PSSocket::Get_* in AcerAgentService.
    enum Sensor {
        SensorCpuTemp = 0x01, SensorCpuFan = 0x02, SensorSysTemp = 0x03, SensorSys1Fan = 0x04,
        SensorGpu1Fan = 0x06, SensorSys2Fan = 0x08, SensorGpu2Fan = 0x09,
        SensorGpu1Temp = 0x0A, SensorGpu2Temp = 0x0B,
    };

    // Fan IDs of SetGamingFanSpeed / GetGamingFanSpeed.
    static constexpr int FanIdCpu = 1;
    static constexpr int FanIdGpuBase = 4; // GPU fan n -> 4 + n

    struct FanCount { int cpu = 0, gpu = 0, sys = 0; };

    struct FanSetting {
        bool autoMode = true; // fan_custom_auto
        int speed = 50;       // fan_custom_speed, percent
    };

    struct KbConfig {          // SetGamingKBBacklight byte layout
        int effect = 0;        // [0] 0 static,1 breathing,2 neon,3 wave,4 shifting,5 zoom,6 meteor,7 twinkling
        int speed = 0;         // [1] 1..9 (0 for static)
        int brightness = 100;  // [2] 0..100
        int direction = 1;     // [4] 1 right-to-left, 2 left-to-right
        QColor color;          // [5..7]
    };

    explicit AcerHardware(WmiDevice &wmi) : m_wmi(wmi) {}

    // ---- operating mode (TsClientCommandProcessor::set_operation_mode)
    std::optional<int> operatingMode();
    bool setOperatingMode(int mode);
    QList<int> supportedModes();

    // ---- sensors / fans
    std::optional<uint16_t> supportedSensors();
    std::optional<int> readSensor(int sensorId);
    FanCount fanCount();
    bool setFanMode(FanMode mode, const FanCount &fans);
    bool setFanCustom(const FanSetting &cpu, const QList<FanSetting> &gpu, const FanCount &fans);
    std::optional<int> fanBehaviorOf(int fanIndexInGroup, bool gpu); // 1 auto, 2 max, 3 custom
    std::optional<int> fanSpeedSetting(int fanId);

    // ---- EC keyboard lighting (ECKeyboard in AcerLightingService)
    bool setKbBacklight(const KbConfig &cfg);
    // ledBehaviorSize: 16 (SMBIOS 172 v>=0x5B), 12 (v 0x56..0x5A) or 8 (no table).
    bool setKbZoneColors(int brightness, const QList<QColor> &zones, int ledBehaviorSize);
    std::optional<KbConfig> kbBacklight();

    // ---- keyboard backlight auto-off after 30 s (APGeAction, BK hotkey from SMBIOS type 170)
    std::optional<bool> kbTimeout(int bkHotkey);
    bool setKbTimeout(int bkHotkey, bool enabled);

    // ---- misc settings
    std::optional<bool> bootSound();             // GetGamingMiscSetting(6)
    bool setBootSound(bool on);                  // SetGamingMiscSetting(6 | on<<8)
    std::optional<int> lcdOverdriveRaw();        // GetGamingProfile(0) byte 6 (0xFF = unsupported)
    bool setLcdOverdrive(bool on);               // SetGamingProfile(0x10 | on<<48)

    // ---- power-off USB charging (APGeAction function 4, AcerQAAgent)
    struct UsbCharge { bool enabled = false; int threshold = 10; };
    std::optional<UsbCharge> usbCharging();
    bool setUsbCharging(const UsbCharge &u);

    // ---- battery (BatteryControl, AcerQAAgent/AcerCCAgent)
    struct BatteryHealth { bool healthMode = false; bool calibration = false; };
    std::optional<BatteryHealth> batteryHealth();
    bool setBatteryFunction(int function, bool enabled); // 1 = 80% limit, 2 = calibration
    std::optional<int> batteryBoostRaw();         // GetGamingSysInfo(2) byte 5

private:
    std::optional<uint64_t> gaming32(unsigned method, uint32_t in);
    bool gamingSet64(unsigned method, uint64_t in);
    WmiDevice &m_wmi;
};
