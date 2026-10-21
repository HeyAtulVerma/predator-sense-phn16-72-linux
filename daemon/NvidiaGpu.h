#pragma once

#include <QString>
#include <optional>

// NVIDIA dGPU access through NVML (loaded at runtime, optional).
//
// PredatorSense uses NVAPI to (a) read GPU usage/clock for the monitor page and
// (b) shift core/memory clocks when the operating mode changes (model INI
// [OC_GPU]). NVML offers the same on Linux. Like the Windows service
// (GpuGetIdleState / GPU1_IDLE_STATUS) we never wake a runtime-suspended dGPU
// just to read numbers.
class NvidiaGpu
{
public:
    struct Stats {
        bool idle = true;       // runtime suspended -> no numbers
        int usage = -1;         // percent
        int coreMHz = -1, memMHz = -1, maxCoreMHz = -1;
        int tempC = -1;
        double powerW = -1;
    };

    NvidiaGpu();
    ~NvidiaGpu();

    bool present() const { return !m_pciAddr.isEmpty(); }
    QString pciAddress() const { return m_pciAddr; }
    QString pciId() const { return m_pciId; }       // "PCI\\VEN_10DE&DEV_28E0&SUBSYS_17311025"
    bool runtimeActive() const;
    Stats stats();

    // Core/memory clock offsets in MHz (NVAPI freqDelta / 1000 on Windows).
    bool setClockOffsets(int coreMHz, int memMHz, QString *error = nullptr);

private:
    bool ensureNvml();
    void shutdownNvml();

    QString m_pciAddr;   // 0000:01:00.0
    QString m_pciId;
    void *m_lib = nullptr;
    bool m_inited = false;
    void *m_dev = nullptr;
};
