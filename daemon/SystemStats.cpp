#include "SystemStats.h"

#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

static QByteArray readAll(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return {};
    return f.readAll();
}

SystemStats::Snapshot SystemStats::sample()
{
    Snapshot s;

    // CPU usage from the aggregate line of /proc/stat
    const QByteArray stat = readAll(QStringLiteral("/proc/stat"));
    const QList<QByteArray> fields = stat.left(stat.indexOf('\n')).simplified().split(' ');
    if (fields.size() >= 8 && fields[0] == "cpu") {
        uint64_t total = 0;
        for (int i = 1; i < fields.size(); ++i)
            total += fields[i].toULongLong();
        const uint64_t idle = fields[4].toULongLong() + fields[5].toULongLong(); // idle + iowait
        if (m_prevTotal && total > m_prevTotal) {
            const double dt = double(total - m_prevTotal);
            s.cpuUsage = 100.0 * (1.0 - double(idle - m_prevIdle) / dt);
        }
        m_prevIdle = idle;
        m_prevTotal = total;
    }

    // Per-core frequencies
    QDir cpuDir(QStringLiteral("/sys/devices/system/cpu"));
    static const QRegularExpression cpuRe(QStringLiteral("^cpu[0-9]+$"));
    QStringList cpus = cpuDir.entryList({QStringLiteral("cpu*")}, QDir::Dirs);
    std::sort(cpus.begin(), cpus.end(), [](const QString &a, const QString &b) {
        return a.mid(3).toInt() < b.mid(3).toInt();
    });
    int64_t sum = 0;
    for (const QString &c : cpus) {
        if (!cpuRe.match(c).hasMatch())
            continue;
        const int khz = readAll(cpuDir.filePath(c + QStringLiteral("/cpufreq/scaling_cur_freq"))).trimmed().toInt();
        if (khz <= 0)
            continue;
        s.coreFreqMHz << khz / 1000;
        sum += khz / 1000;
        const int maxKhz = readAll(cpuDir.filePath(c + QStringLiteral("/cpufreq/cpuinfo_max_freq"))).trimmed().toInt();
        s.cpuMaxFreqMHz = std::max(s.cpuMaxFreqMHz, maxKhz / 1000);
    }
    if (!s.coreFreqMHz.isEmpty())
        s.cpuFreqMHz = int(sum / s.coreFreqMHz.size());

    // Memory
    const QByteArray mem = readAll(QStringLiteral("/proc/meminfo"));
    uint64_t totalKb = 0, availKb = 0;
    for (const QByteArray &line : mem.split('\n')) {
        if (line.startsWith("MemTotal:"))
            totalKb = line.simplified().split(' ').value(1).toULongLong();
        else if (line.startsWith("MemAvailable:"))
            availKb = line.simplified().split(' ').value(1).toULongLong();
    }
    if (totalKb) {
        s.ramTotalGB = double(totalKb) / (1024.0 * 1024.0);
        s.ramUsage = 100.0 * double(totalKb - availKb) / double(totalKb);
    }
    return s;
}
