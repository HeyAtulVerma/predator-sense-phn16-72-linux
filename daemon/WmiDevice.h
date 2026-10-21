#pragma once

#include <QByteArray>
#include <QHash>
#include <QString>
#include <cstdint>
#include <optional>

// Thin wrapper over /dev/predator-wmi (kernel/predator_wmi.c).
class WmiDevice
{
public:
    enum Iface : uint32_t { Gaming = 0, APGe = 1, Battery = 2 };

    struct Result {
        QByteArray data;   // raw bytes returned by the ACPI method
        bool isInteger = false;
        uint64_t u64() const; // little-endian value of the first <= 8 bytes
    };

    WmiDevice() = default;
    ~WmiDevice();
    WmiDevice(const WmiDevice &) = delete;
    WmiDevice &operator=(const WmiDevice &) = delete;

    bool open(const QString &path = QStringLiteral("/dev/predator-wmi"));
    // Development aid: answer calls from an in-memory model of the firmware.
    void enableSimulation();
    bool isOpen() const { return m_fd >= 0 || m_sim; }
    bool hasIface(Iface i) const { return m_ifaceMask & (1u << i); }
    QString lastError() const { return m_lastError; }

    std::optional<Result> call(Iface iface, uint32_t methodId, const QByteArray &input);

    // Integer arguments are marshalled with the width the MOF declares,
    // exactly like the Windows WMI provider does (uint32 = 4 bytes, uint64 = 8).
    std::optional<uint64_t> callU32(Iface iface, uint32_t methodId, uint32_t in);
    std::optional<uint64_t> callU64(Iface iface, uint32_t methodId, uint64_t in);

private:
    std::optional<Result> simulate(Iface iface, uint32_t methodId, const QByteArray &input);
    bool m_sim = false;
    QHash<QString, uint64_t> m_simState;
    int m_fd = -1;
    uint32_t m_ifaceMask = 0;
    QString m_lastError;
};
