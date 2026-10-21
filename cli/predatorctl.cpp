// predatorctl - command line client of predator-sensed
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusVariant>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include <cstdio>

static QDBusConnection predatorBus()
{
    // PREDATOR_SENSE_BUS=session talks to "predator-sensed --simulate"
    return qgetenv("PREDATOR_SENSE_BUS") == "session" ? QDBusConnection::sessionBus() : QDBusConnection::systemBus();
}

static const char *usage =
    "Usage: predatorctl <command> [args]\n"
    "\n"
    "  status                         current settings\n"
    "  info                           detected hardware capabilities\n"
    "  sensors [--watch]              temperatures, fans, usage\n"
    "  mode <quiet|balanced|performance|turbo|eco>\n"
    "  battery-mode <balanced|eco>    mode used while on battery\n"
    "  fan auto | fan max\n"
    "  fan custom <cpu%|auto> <gpu%|auto>\n"
    "  kb static <#c1> [#c2 #c3 #c4] [--brightness N]\n"
    "  kb <breathing|neon|wave|shifting|zoom|meteor|twinkling> [--color #rgb] [--speed 1-9]\n"
    "                                 [--brightness 0-100] [--direction left|right]\n"
    "  kb off\n"
    "  set <option> <value>           bootSound, lcdOverdrive, kbTimeout, usbCharging,\n"
    "                                 usbChargingThreshold (10/20/30), batteryLimit,\n"
    "                                 batteryCalibration, gpuOcInModes\n";

static int modeValue(const QString &n)
{
    if (n == QLatin1String("quiet")) return 0;
    if (n == QLatin1String("balanced")) return 1;
    if (n == QLatin1String("performance")) return 4;
    if (n == QLatin1String("turbo")) return 5;
    if (n == QLatin1String("eco")) return 6;
    return -1;
}

static void printJson(const QString &s)
{
    std::printf("%s", QJsonDocument::fromJson(s.toUtf8()).toJson(QJsonDocument::Indented).constData());
}

static int fail(const QString &msg)
{
    std::fprintf(stderr, "predatorctl: %s\n", qPrintable(msg));
    return 1;
}

template <typename... A> static int callVoid(QDBusInterface &iface, const char *method, A... args)
{
    QDBusMessage r = iface.call(QString::fromLatin1(method), args...);
    if (r.type() == QDBusMessage::ErrorMessage)
        return fail(r.errorMessage());
    return 0;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QStringList a = app.arguments().mid(1);
    if (a.isEmpty() || a[0] == QLatin1String("-h") || a[0] == QLatin1String("--help")) {
        std::printf("%s", usage);
        return a.isEmpty() ? 1 : 0;
    }

    QDBusInterface iface(QStringLiteral("org.predatorsense.Daemon"), QStringLiteral("/org/predatorsense/Daemon"),
                         QStringLiteral("org.predatorsense.Daemon1"), predatorBus());
    iface.setTimeout(130000);
    if (!iface.isValid())
        return fail(QStringLiteral("cannot reach predator-sensed (is the service running?)"));

    const QString cmd = a.takeFirst();
    auto opt = [&](const QString &name, const QString &def = {}) {
        const int i = a.indexOf(name);
        if (i < 0 || i + 1 >= a.size())
            return def;
        const QString v = a[i + 1];
        a.remove(i, 2);
        return v;
    };

    if (cmd == QLatin1String("status") || cmd == QLatin1String("info") || cmd == QLatin1String("sensors")) {
        const char *m = cmd == QLatin1String("status") ? "GetState" : cmd == QLatin1String("info") ? "GetInfo" : "GetSensors";
        do {
            QDBusReply<QString> r = iface.call(QString::fromLatin1(m));
            if (!r.isValid())
                return fail(r.error().message());
            printJson(r.value());
            if (!a.contains(QLatin1String("--watch")))
                break;
            QThread::sleep(2);
        } while (true);
        return 0;
    }
    if (cmd == QLatin1String("mode") && a.size() == 1) {
        const int m = modeValue(a[0]);
        return m < 0 ? fail(QStringLiteral("unknown mode")) : callVoid(iface, "SetOperatingMode", m);
    }
    if (cmd == QLatin1String("battery-mode") && a.size() == 1) {
        const int m = modeValue(a[0]);
        return m < 0 ? fail(QStringLiteral("unknown mode")) : callVoid(iface, "SetBatteryMode", m);
    }
    if (cmd == QLatin1String("fan") && !a.isEmpty()) {
        QJsonObject f{{QStringLiteral("mode"), a[0]}};
        if (a[0] == QLatin1String("custom")) {
            if (a.size() < 3)
                return fail(QStringLiteral("fan custom <cpu%|auto> <gpu%|auto>"));
            auto fanObj = [](const QString &v) {
                return v == QLatin1String("auto") ? QJsonObject{{QStringLiteral("auto"), true}}
                                                  : QJsonObject{{QStringLiteral("auto"), false}, {QStringLiteral("speed"), v.toInt()}};
            };
            QJsonArray gpu;
            for (int i = 2; i < a.size(); ++i)
                gpu.append(fanObj(a[i]));
            f.insert(QStringLiteral("cpu"), fanObj(a[1]));
            f.insert(QStringLiteral("gpu"), gpu);
        }
        return callVoid(iface, "SetFanControl", QString::fromUtf8(QJsonDocument(f).toJson(QJsonDocument::Compact)));
    }
    if (cmd == QLatin1String("kb") && !a.isEmpty()) {
        QJsonObject l;
        const QString b = opt(QStringLiteral("--brightness"));
        const QString c = opt(QStringLiteral("--color"));
        const QString s = opt(QStringLiteral("--speed"));
        const QString d = opt(QStringLiteral("--direction"));
        const QString effect = a.takeFirst();
        if (effect == QLatin1String("off")) {
            l.insert(QStringLiteral("brightness"), 0);
        } else {
            l.insert(QStringLiteral("effect"), effect);
            if (effect == QLatin1String("static") && !a.isEmpty()) {
                QJsonArray zones;
                for (int i = 0; i < 4; ++i)
                    zones.append(a.value(i, a.last()));
                l.insert(QStringLiteral("zones"), zones);
            }
            if (b.isEmpty())
                l.insert(QStringLiteral("brightness"), 100);
        }
        if (!b.isEmpty()) l.insert(QStringLiteral("brightness"), b.toInt());
        if (!c.isEmpty()) l.insert(QStringLiteral("color"), c);
        if (!s.isEmpty()) l.insert(QStringLiteral("speed"), s.toInt());
        if (!d.isEmpty()) l.insert(QStringLiteral("direction"), d == QLatin1String("left") ? 2 : 1);
        return callVoid(iface, "SetKeyboardLighting", QString::fromUtf8(QJsonDocument(l).toJson(QJsonDocument::Compact)));
    }
    if (cmd == QLatin1String("set") && a.size() == 2) {
        QVariant v;
        const QString s = a[1].toLower();
        if (a[0] == QLatin1String("usbChargingThreshold"))
            v = s.toInt();
        else if (s == QLatin1String("on") || s == QLatin1String("true") || s == QLatin1String("1"))
            v = true;
        else if (s == QLatin1String("off") || s == QLatin1String("false") || s == QLatin1String("0"))
            v = false;
        else
            return fail(QStringLiteral("value must be on/off"));
        return callVoid(iface, "SetOption", a[0], QVariant::fromValue(QDBusVariant(v)));
    }
    std::printf("%s", usage);
    return 1;
}
