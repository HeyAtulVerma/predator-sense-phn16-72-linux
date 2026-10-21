#include "NvidiaGpu.h"

#include <QDir>
#include <QFile>
#include <QLoggingCategory>
#include <dlfcn.h>

Q_LOGGING_CATEGORY(lcGpu, "predator.gpu")

namespace {
using nvmlReturn_t = int;
using nvmlDevice_t = void *;
constexpr nvmlReturn_t NVML_SUCCESS = 0;
constexpr int NVML_CLOCK_GRAPHICS = 0;
constexpr int NVML_CLOCK_MEM = 2;
constexpr int NVML_TEMPERATURE_GPU = 0;
constexpr int NVML_PSTATE_0 = 0;

struct nvmlUtilization_t { unsigned int gpu, memory; };
struct nvmlClockOffset_v1_t {
    unsigned int version;
    int type;
    int pstate;
    int clockOffsetMHz;
    int minClockOffsetMHz;
    int maxClockOffsetMHz;
};
constexpr unsigned int nvmlClockOffset_v1 = unsigned(sizeof(nvmlClockOffset_v1_t)) | (1u << 24);

QByteArray readFile(const QString &p)
{
    QFile f(p);
    return f.open(QIODevice::ReadOnly) ? f.readAll().trimmed() : QByteArray();
}

template <typename F> F sym(void *lib, const char *name)
{
    return lib ? reinterpret_cast<F>(dlsym(lib, name)) : nullptr;
}
} // namespace

NvidiaGpu::NvidiaGpu()
{
    QDir pci(QStringLiteral("/sys/bus/pci/devices"));
    for (const QString &d : pci.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        const QString base = pci.filePath(d);
        if (readFile(base + QStringLiteral("/vendor")) != "0x10de")
            continue;
        if (!readFile(base + QStringLiteral("/class")).startsWith("0x03"))
            continue;
        m_pciAddr = d;
        auto hex4 = [&](const char *f) {
            return QString::fromLatin1(readFile(base + QLatin1Char('/') + QLatin1String(f))).mid(2).toUpper();
        };
        m_pciId = QStringLiteral("PCI\\VEN_%1&DEV_%2&SUBSYS_%3%4")
                      .arg(hex4("vendor"), hex4("device"), hex4("subsystem_device"), hex4("subsystem_vendor"));
        break;
    }
}

NvidiaGpu::~NvidiaGpu()
{
    shutdownNvml();
    if (m_lib)
        dlclose(m_lib);
}

bool NvidiaGpu::runtimeActive() const
{
    if (m_pciAddr.isEmpty())
        return false;
    const QByteArray st = readFile(QStringLiteral("/sys/bus/pci/devices/%1/power/runtime_status").arg(m_pciAddr));
    return st.isEmpty() || st == "active";
}

bool NvidiaGpu::ensureNvml()
{
    if (m_inited)
        return true;
    if (!m_lib)
        m_lib = dlopen("libnvidia-ml.so.1", RTLD_NOW | RTLD_LOCAL);
    if (!m_lib)
        return false;
    auto init = sym<nvmlReturn_t (*)()>(m_lib, "nvmlInit_v2");
    auto byPci = sym<nvmlReturn_t (*)(const char *, nvmlDevice_t *)>(m_lib, "nvmlDeviceGetHandleByPciBusId_v2");
    if (!init || !byPci || init() != NVML_SUCCESS)
        return false;
    m_inited = true;
    if (byPci(m_pciAddr.toLatin1().constData(), &m_dev) != NVML_SUCCESS) {
        shutdownNvml();
        return false;
    }
    return true;
}

void NvidiaGpu::shutdownNvml()
{
    if (!m_inited)
        return;
    if (auto shut = sym<nvmlReturn_t (*)()>(m_lib, "nvmlShutdown"))
        shut();
    m_inited = false;
    m_dev = nullptr;
}

NvidiaGpu::Stats NvidiaGpu::stats()
{
    Stats s;
    if (!present() || !runtimeActive()) {
        // Release NVML so the GPU can stay in D3cold.
        shutdownNvml();
        return s;
    }
    if (!ensureNvml())
        return s;
    s.idle = false;
    unsigned v = 0;
    if (auto f = sym<nvmlReturn_t (*)(nvmlDevice_t, nvmlUtilization_t *)>(m_lib, "nvmlDeviceGetUtilizationRates")) {
        nvmlUtilization_t u{};
        if (f(m_dev, &u) == NVML_SUCCESS)
            s.usage = int(u.gpu);
    }
    if (auto f = sym<nvmlReturn_t (*)(nvmlDevice_t, int, unsigned *)>(m_lib, "nvmlDeviceGetClockInfo")) {
        if (f(m_dev, NVML_CLOCK_GRAPHICS, &v) == NVML_SUCCESS) s.coreMHz = int(v);
        if (f(m_dev, NVML_CLOCK_MEM, &v) == NVML_SUCCESS) s.memMHz = int(v);
    }
    if (auto f = sym<nvmlReturn_t (*)(nvmlDevice_t, int, unsigned *)>(m_lib, "nvmlDeviceGetMaxClockInfo"))
        if (f(m_dev, NVML_CLOCK_GRAPHICS, &v) == NVML_SUCCESS) s.maxCoreMHz = int(v);
    if (auto f = sym<nvmlReturn_t (*)(nvmlDevice_t, int, unsigned *)>(m_lib, "nvmlDeviceGetTemperature"))
        if (f(m_dev, NVML_TEMPERATURE_GPU, &v) == NVML_SUCCESS) s.tempC = int(v);
    if (auto f = sym<nvmlReturn_t (*)(nvmlDevice_t, unsigned *)>(m_lib, "nvmlDeviceGetPowerUsage"))
        if (f(m_dev, &v) == NVML_SUCCESS) s.powerW = v / 1000.0;
    return s;
}

bool NvidiaGpu::setClockOffsets(int coreMHz, int memMHz, QString *error)
{
    auto fail = [&](const QString &e) {
        if (error) *error = e;
        qCWarning(lcGpu) << e;
        return false;
    };
    if (!present())
        return fail(QStringLiteral("no NVIDIA GPU"));
    if (!ensureNvml())
        return fail(QStringLiteral("NVML not available"));

    // Driver >= 555: nvmlDeviceSetClockOffsets; older: the VF-offset calls.
    if (auto f = sym<nvmlReturn_t (*)(nvmlDevice_t, nvmlClockOffset_v1_t *)>(m_lib, "nvmlDeviceSetClockOffsets")) {
        nvmlClockOffset_v1_t o{};
        o.version = nvmlClockOffset_v1;
        o.pstate = NVML_PSTATE_0;
        o.type = NVML_CLOCK_GRAPHICS;
        o.clockOffsetMHz = coreMHz;
        const nvmlReturn_t a = f(m_dev, &o);
        o.type = NVML_CLOCK_MEM;
        o.clockOffsetMHz = memMHz;
        const nvmlReturn_t b = f(m_dev, &o);
        if (a == NVML_SUCCESS && b == NVML_SUCCESS)
            return true;
        qCWarning(lcGpu) << "nvmlDeviceSetClockOffsets returned" << a << b << "- trying VF offsets";
    }
    auto gpc = sym<nvmlReturn_t (*)(nvmlDevice_t, int)>(m_lib, "nvmlDeviceSetGpcClkVfOffset");
    auto mem = sym<nvmlReturn_t (*)(nvmlDevice_t, int)>(m_lib, "nvmlDeviceSetMemClkVfOffset");
    if (!gpc || !mem)
        return fail(QStringLiteral("driver does not expose clock offset control"));
    const nvmlReturn_t a = gpc(m_dev, coreMHz);
    const nvmlReturn_t b = mem(m_dev, memMHz);
    if (a != NVML_SUCCESS || b != NVML_SUCCESS)
        return fail(QStringLiteral("NVML clock offset error %1/%2").arg(a).arg(b));
    return true;
}
