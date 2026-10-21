#include "ModelConfig.h"

#include <QDir>
#include <QFile>

ModelConfig ModelConfig::load(const QString &dir, const QString &productName)
{
    ModelConfig c;
    QFile f(QDir(dir).filePath(productName + QStringLiteral(".ini")));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return c;
    QString section;
    while (!f.atEnd()) {
        const QString line = QString::fromUtf8(f.readLine()).trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char(';')) || line.startsWith(QLatin1Char('#')))
            continue;
        if (line.startsWith(QLatin1Char('[')) && line.endsWith(QLatin1Char(']'))) {
            section = line.mid(1, line.size() - 2).trimmed().toUpper();
            continue;
        }
        const int eq = line.indexOf(QLatin1Char('='));
        if (eq > 0)
            c.m_sections[section][line.left(eq).trimmed().toUpper()] = line.mid(eq + 1).trimmed();
    }
    c.m_loaded = true;
    return c;
}

std::optional<ModelConfig::GpuOc> ModelConfig::gpuOc(const QString &pciId) const
{
    const Section oc = m_sections.value(QStringLiteral("OC_GPU"));
    const int count = oc.value(QStringLiteral("COUNT")).toInt();
    const QString want = pciId.toUpper();
    for (int i = 1; i <= count; ++i) {
        if (oc.value(QStringLiteral("GPUNAME%1").arg(i)).toUpper() != want)
            continue;
        const Section s = m_sections.value(want);
        // Newer INIs: Quiet/Balanced/Performance/Turbo, older: Quiet/Default/Extreme/Turbo
        static const char *names[4][2] = {{"QUIET", "QUIET"}, {"BALANCED", "DEFAULT"},
                                          {"PERFORMANCE", "EXTREME"}, {"TURBO", "TURBO"}};
        GpuOc g;
        for (int l = 0; l < 4; ++l) {
            for (const char *n : names[l]) {
                const QString key = QString::fromLatin1(n);
                if (s.contains(key)) g.core[l] = s.value(key).toInt();
                if (s.contains(QStringLiteral("RAM_") + key)) g.mem[l] = s.value(QStringLiteral("RAM_") + key).toInt();
            }
        }
        return g;
    }
    return std::nullopt;
}

std::optional<int> ModelConfig::fanSupport(const QString &key) const
{
    const Section s = m_sections.value(QStringLiteral("FANSUPPORT"));
    if (!s.contains(key.toUpper()))
        return std::nullopt;
    return s.value(key.toUpper()).toInt();
}

int ModelConfig::ocLevelForMode(int mode)
{
    // Disassembly of set_operation_mode (0x14000d34b..0x14000d37b): Quiet calls
    // gpu_set_oc_level(0) and then falls through to level 1, so the final level
    // for Quiet is 1. Eco keeps the initial value 0.
    switch (mode) {
    case 6: return 0; // Eco
    case 4: return 2; // Performance
    case 5: return 3; // Turbo
    default: return 1; // Quiet (net effect), Balanced
    }
}
