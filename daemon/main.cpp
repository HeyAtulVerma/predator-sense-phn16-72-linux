#include "DBusService.h"
#include "Daemon.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusError>
#include <QDir>
#include <cstdio>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("predator-sensed"));
    app.setApplicationVersion(QStringLiteral(PREDATOR_VERSION));

    QCommandLineParser p;
    p.setApplicationDescription(QStringLiteral("Predator Sense for Linux - system service"));
    p.addHelpOption();
    p.addVersionOption();
    QCommandLineOption state({QStringLiteral("state")}, QStringLiteral("State file."), QStringLiteral("path"));
    QCommandLineOption models({QStringLiteral("models")}, QStringLiteral("Model INI directory."), QStringLiteral("dir"));
    QCommandLineOption device({QStringLiteral("device")}, QStringLiteral("WMI device node."), QStringLiteral("path"));
    QCommandLineOption simulate({QStringLiteral("simulate")},
                                QStringLiteral("Use a simulated firmware on the session bus (development)."));
    p.addOptions({state, models, device, simulate});
    p.process(app);

    Daemon::Options opt;
    if (p.isSet(state)) opt.statePath = p.value(state);
    if (p.isSet(models)) opt.modelDir = p.value(models);
    if (p.isSet(device)) opt.devicePath = p.value(device);
    if (p.isSet(simulate)) {
        opt.simulate = true;
        if (!p.isSet(state))
            opt.statePath = QDir::tempPath() + QStringLiteral("/predator-sense-sim-state.json");
        if (!p.isSet(models))
            opt.modelDir = QStringLiteral(PREDATOR_SOURCE_DIR "/data/models");
    }

    auto *daemon = new Daemon(opt, &app);
    QString error;
    if (!daemon->start(&error)) {
        std::fprintf(stderr, "predator-sensed: %s\n", qPrintable(error));
        return 1;
    }

    auto *svc = new DBusService(daemon, !opt.simulate);
    QDBusConnection bus = opt.simulate ? QDBusConnection::sessionBus() : QDBusConnection::systemBus();
    if (!bus.registerObject(QStringLiteral("/org/predatorsense/Daemon"), svc,
                            QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals) ||
        !bus.registerService(QStringLiteral("org.predatorsense.Daemon"))) {
        std::fprintf(stderr, "predator-sensed: cannot register on the system bus: %s\n",
                     qPrintable(bus.lastError().message()));
        return 1;
    }
    return app.exec();
}
