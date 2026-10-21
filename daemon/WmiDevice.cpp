#include "WmiDevice.h"

#include "../kernel/predator_wmi_uapi.h"

#include <QDateTime>
#include <QtEndian>
#include <cmath>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

uint64_t WmiDevice::Result::u64() const
{
    uint8_t buf[8] = {};
    std::memcpy(buf, data.constData(), std::min<qsizetype>(data.size(), 8));
    return qFromLittleEndian<quint64>(buf);
}

WmiDevice::~WmiDevice()
{
    if (m_fd >= 0)
        ::close(m_fd);
}

bool WmiDevice::open(const QString &path)
{
    if (m_fd >= 0)
        return true;
    m_fd = ::open(path.toLocal8Bit().constData(), O_RDWR | O_CLOEXEC);
    if (m_fd < 0) {
        m_lastError = QStringLiteral("cannot open %1: %2").arg(path, QString::fromLocal8Bit(strerror(errno)));
        return false;
    }
    predator_wmi_info info{};
    if (::ioctl(m_fd, PREDATOR_WMI_IOC_INFO, &info) < 0 || info.api_version != PREDATOR_WMI_API_VERSION) {
        m_lastError = QStringLiteral("predator_wmi API mismatch");
        ::close(m_fd);
        m_fd = -1;
        return false;
    }
    m_ifaceMask = info.iface_mask;
    return true;
}

std::optional<WmiDevice::Result> WmiDevice::call(Iface iface, uint32_t methodId, const QByteArray &input)
{
    if (m_sim)
        return simulate(iface, methodId, input);
    if (m_fd < 0 || input.size() > PREDATOR_WMI_MAX_IN)
        return std::nullopt;

    predator_wmi_call c{};
    c.iface = iface;
    c.method_id = methodId;
    c.in_len = uint32_t(input.size());
    std::memcpy(c.in, input.constData(), input.size());

    if (::ioctl(m_fd, PREDATOR_WMI_IOC_CALL, &c) < 0) {
        m_lastError = QStringLiteral("WMI call iface=%1 method=%2 failed: %3")
                          .arg(iface).arg(methodId).arg(QString::fromLocal8Bit(strerror(errno)));
        return std::nullopt;
    }
    Result r;
    r.isInteger = c.out_type == PREDATOR_WMI_OUT_INTEGER;
    r.data = QByteArray(reinterpret_cast<const char *>(c.out), int(c.out_len));
    return r;
}

std::optional<uint64_t> WmiDevice::callU32(Iface iface, uint32_t methodId, uint32_t in)
{
    uint8_t buf[4];
    qToLittleEndian<quint32>(in, buf);
    auto r = call(iface, methodId, QByteArray(reinterpret_cast<char *>(buf), 4));
    if (!r)
        return std::nullopt;
    return r->u64();
}

std::optional<uint64_t> WmiDevice::callU64(Iface iface, uint32_t methodId, uint64_t in)
{
    uint8_t buf[8];
    qToLittleEndian<quint64>(in, buf);
    auto r = call(iface, methodId, QByteArray(reinterpret_cast<char *>(buf), 8));
    if (!r)
        return std::nullopt;
    return r->u64();
}

// ------------------------------------------------------------------ simulation

void WmiDevice::enableSimulation()
{
    m_sim = true;
    m_ifaceMask = 0x7;
    m_simState = {{QStringLiteral("mode"), 1}, {QStringLiteral("boot"), 1}, {QStringLiteral("lcd"), 0},
                  {QStringLiteral("usb"), (10u << 16) | (0x1Fu << 8)}, {QStringLiteral("kbto"), 0},
                  {QStringLiteral("health"), 0}, {QStringLiteral("calib"), 0}, {QStringLiteral("fanbeh"), 0}};
}

std::optional<WmiDevice::Result> WmiDevice::simulate(Iface iface, uint32_t m, const QByteArray &in)
{
    uint8_t buf[8] = {};
    std::memcpy(buf, in.constData(), std::min<qsizetype>(in.size(), 8));
    const uint64_t v = qFromLittleEndian<quint64>(buf);
    auto &st = m_simState;
    auto integer = [](uint64_t x) {
        Result r;
        r.isInteger = true;
        uint8_t b[8];
        qToLittleEndian<quint64>(x, b);
        r.data = QByteArray(reinterpret_cast<char *>(b), 8);
        return r;
    };
    const double t = double(QDateTime::currentMSecsSinceEpoch() % 600000) / 1000.0;
    if (iface == Gaming) {
        switch (m) {
        case 5: { // GetGamingSysInfo
            if (v == 0)
                return integer(uint64_t(0x0227) << 24); // cpu temp/fan, sys temp, gpu fan, gpu temp
            const int id = int(v >> 8);
            const int mode = int(st.value(QStringLiteral("mode")));
            const double load = mode == 5 ? 1.0 : mode == 4 ? 0.8 : mode == 0 ? 0.35 : 0.55;
            int val = 0;
            switch (id) {
            case 1: val = int(52 + 25 * load + 6 * std::sin(t / 7)); break;
            case 3: val = int(41 + 4 * std::sin(t / 13)); break;
            case 10: val = int(48 + 20 * load + 5 * std::sin(t / 9)); break;
            case 2: case 6: {
                const uint64_t beh = st.value(QStringLiteral("fanbeh"));
                const bool max = (beh & 0x3) == 2;
                val = max ? 6800 : int(2400 + 3400 * load + 150 * std::sin(t / 5 + id));
                break;
            }
            default: return integer(1); // unsupported
            }
            return integer(uint64_t(val) << 8);
        }
        case 23: { // GetGamingMiscSetting
            switch (v) {
            case 0x0B: return integer(st.value(QStringLiteral("mode")) << 8);
            case 0x0A: return integer(uint64_t(0x73) << 8);
            case 0x06: return integer(st.value(QStringLiteral("boot")) << 8);
            default: return integer(1);
            }
        }
        case 22: // SetGamingMiscSetting
            if ((v & 0xFF) == 0x0B) st[QStringLiteral("mode")] = (v >> 8) & 0xFF;
            else if ((v & 0xFF) == 0x06) st[QStringLiteral("boot")] = (v >> 8) & 0xFF;
            return integer(0);
        case 3: return integer(uint64_t(0xFF) << 48); // GetGamingProfile: no LCD overdrive
        case 14: st[QStringLiteral("fanbeh")] = v >> 16; return integer(0);
        case 15: return integer(st.value(QStringLiteral("fanbeh")) << 8);
        case 16: st[QStringLiteral("fan%1").arg(v & 0xFF)] = (v >> 8) & 0xFF; return integer(0);
        case 17: return integer(st.value(QStringLiteral("fan%1").arg(v & 0xFF), 50) << 8);
        case 1: case 2: case 6: case 20: return integer(0);
        case 21: { Result r; r.data = QByteArray(16, '\0'); return r; }
        }
        return integer(1);
    }
    if (iface == APGe) {
        if (m == 2) {
            if (v == 4) return integer(st.value(QStringLiteral("usb")));
            if ((v & 0xFF00FF) == 0x080001) return integer((st.value(QStringLiteral("kbto")) ? 0x1Eull : 0) << 40 | 100ull << 32);
            return integer(1);
        }
        if (m == 1) {
            if ((v & 0xFF) == 4) st[QStringLiteral("usb")] = v & 0xFFFF00;
            if ((v & 0xFF00FF) == 0x080002) st[QStringLiteral("kbto")] = ((v >> 40) & 0xFF) == 0x1E;
            return integer(0);
        }
    }
    if (iface == Battery) {
        if (m == 20) {
            Result r;
            r.data = QByteArray(8, '\0');
            r.data[0] = 3;
            r.data[3] = char(st.value(QStringLiteral("health")));
            r.data[4] = char(st.value(QStringLiteral("calib")));
            return r;
        }
        if (m == 21) {
            if (in.size() >= 3) st[in[1] == 1 ? QStringLiteral("health") : QStringLiteral("calib")] = quint8(in[2]);
            Result r;
            r.data = QByteArray(4, '\0');
            return r;
        }
    }
    return std::nullopt;
}
