#pragma once

#include <QHash>
#include <QString>
#include <optional>

// Per-model configuration in the same format as the INI files PredatorSense
// ships ("<Product Name>.ini"): [OC_GPU] clock offsets per operating mode and
// optional [FanSupport] fan counts.
class ModelConfig
{
public:
    struct GpuOc { int core[4] = {}; int mem[4] = {}; }; // index: oc level 0..3

    static ModelConfig load(const QString &dir, const QString &productName);

    bool loaded() const { return m_loaded; }
    std::optional<GpuOc> gpuOc(const QString &pciId) const;
    std::optional<int> fanSupport(const QString &key) const; // CPU / GPU / System

    // HardwareService::set_operation_mode -> gpu_set_oc_level
    static int ocLevelForMode(int mode);

private:
    using Section = QHash<QString, QString>;
    QHash<QString, Section> m_sections; // keys upper-cased
    bool m_loaded = false;
};
