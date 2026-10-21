#pragma once

#include "AcerHardware.h"
#include "ModelConfig.h"
#include "NvidiaGpu.h"
#include "SmbiosOem.h"
#include "SystemStats.h"
#include "WmiDevice.h"

#include <QElapsedTimer>
#include <QJsonObject>
#include <QObject>
#include <QTimer>

// The service logic of AcerAgentService, on top of AcerHardware.
class Daemon : public QObject
{
    Q_OBJECT
public:
    struct Options {
        QString statePath = QStringLiteral("/var/lib/predator-sense/state.json");
        QString modelDir = QStringLiteral("/usr/share/predator-sense/models");
        QString devicePath = QStringLiteral("/dev/predator-wmi");
        bool simulate = false;   // fake firmware, session bus (development)
    };

    explicit Daemon(const Options &opt, QObject *parent = nullptr);
    bool start(QString *error);

    QJsonObject info() const;
    QJsonObject state();
    QJsonObject sensors();

    // Setters return an empty string on success or an error message.
    QString setOperatingMode(int mode);
    QString setBatteryMode(int mode);
    QString setFanControl(const QJsonObject &fan);
    QString setKeyboardLighting(const QJsonObject &lighting);
    QString setOption(const QString &key, const QVariant &value);

Q_SIGNALS:
    void stateChanged();

private Q_SLOTS:
    void poll();
    void onPrepareForSleep(bool sleeping);

private:
    void detectCapabilities();
    bool loadState();
    bool checkMode();
    void watchPlatformProfile();
    void saveState();
    bool onAC() const;
    bool applyMode(int mode);
    void afterModeChange(int mode);
    bool applyFan();
    bool applyLighting();
    void applyGpuOc(int mode, bool force);
    void restoreAll(const char *reason);
    bool modeAllowedNow(int mode) const;

    Options m_opt;
    WmiDevice m_wmi;
    AcerHardware m_hw{m_wmi};
    NvidiaGpu m_gpu;
    SystemStats m_stats;
    SmbiosOem m_oem;
    ModelConfig m_model;
    QString m_product;

    // capabilities
    QList<int> m_modes;
    AcerHardware::FanCount m_fans;
    uint16_t m_sensorMask = 0;
    bool m_lcdSupported = false;
    bool m_usbChargeSupported = false;
    bool m_batterySupported = false;
    bool m_bootSoundSupported = false;
    bool m_kbTimeoutSupported = false;
    std::optional<ModelConfig::GpuOc> m_oc;

    // persisted state (what the user chose, like the PredatorSense scenario profile)
    int m_acMode = AcerHardware::Balanced;
    int m_dcMode = AcerHardware::Balanced;
    int m_lastNonTurbo = AcerHardware::Balanced;
    QJsonObject m_fan;       // {mode, cpu:{auto,speed}, gpu:[...]}
    QJsonObject m_lighting;  // {managed, effect, zones, color, speed, brightness, direction}
    bool m_gpuOcEnabled = true;
    bool m_fanManaged = false; // the user changed fans through us at least once

    // runtime
    int m_currentMode = -1;
    int m_lastAC = -1;
    int m_ocAppliedLevel = -1;   // offsets applied during the current GPU power-on period
    bool m_gpuWasActive = false;
    QTimer m_pollTimer;
    int m_pollTick = 0;
    int m_profileFd = -1;
    class QSocketNotifier *m_profileNotifier = nullptr;
    QJsonObject m_sensorCache;
    QElapsedTimer m_sensorAge;
};
