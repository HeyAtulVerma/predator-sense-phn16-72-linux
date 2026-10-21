#pragma once

#include <QList>
#include <cstdint>

// CPU / memory figures shown on the monitoring page (CPU_USAGE, CPU_FREQUENCY,
// CPU_CORE_FREQUENCY, RAM_USAGE, RAM_TOTAL in GET_MONITOR_DATA).
class SystemStats
{
public:
    struct Snapshot {
        double cpuUsage = 0;        // percent
        int cpuFreqMHz = 0;         // average current frequency
        int cpuMaxFreqMHz = 0;
        QList<int> coreFreqMHz;
        double ramUsage = 0;        // percent
        double ramTotalGB = 0;
    };

    Snapshot sample();

private:
    uint64_t m_prevIdle = 0, m_prevTotal = 0;
};
