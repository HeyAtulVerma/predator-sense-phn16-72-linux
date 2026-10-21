#include "Backend.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QIcon>
#include <QMenu>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QUrl>
#include <memory>

// Single-instance helper on the session bus: a second launch raises the window.
class AppService : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.predatorsense.App")
public:
    std::function<void()> onActivate;
public Q_SLOTS:
    void Activate() { if (onActivate) onActivate(); }
};

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("predator-sense"));
    app.setOrganizationName(QStringLiteral("predator-sense"));
    app.setApplicationDisplayName(QStringLiteral("Predator Sense"));
    app.setDesktopFileName(QStringLiteral("predator-sense"));
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/predator-sense.svg")));
    app.setQuitOnLastWindowClosed(false);

    const bool startHidden = app.arguments().contains(QStringLiteral("--hidden"));

    QDBusConnection session = QDBusConnection::sessionBus();
    if (!session.registerService(QStringLiteral("org.predatorsense.App"))) {
        QDBusInterface other(QStringLiteral("org.predatorsense.App"), QStringLiteral("/"),
                             QStringLiteral("org.predatorsense.App"), session);
        other.call(QStringLiteral("Activate"));
        return 0;
    }

    QQuickStyle::setStyle(QStringLiteral("Basic"));
    Backend backend;
    QQmlApplicationEngine engine;
    const bool trayAvailable = QSystemTrayIcon::isSystemTrayAvailable();
    engine.rootContext()->setContextProperty(QStringLiteral("backend"), &backend);
    engine.rootContext()->setContextProperty(QStringLiteral("trayAvailable"), trayAvailable);
    engine.rootContext()->setContextProperty(QStringLiteral("startHidden"), startHidden && trayAvailable);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, [] { QCoreApplication::exit(1); },
                     Qt::QueuedConnection);
    engine.addImportPath(QStringLiteral("qrc:/qt/qml"));
    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/PredatorSense/qml/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return 1;
    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());

    auto showWindow = [window] {
        window->show();
        window->raise();
        window->requestActivate();
    };

    AppService svc;
    svc.onActivate = showWindow;
    session.registerObject(QStringLiteral("/"), &svc, QDBusConnection::ExportAllSlots);

    // ---- tray icon with quick mode switching (like the PredatorSense widget)
    QSystemTrayIcon tray(QIcon(QStringLiteral(":/icons/predator-sense.svg")));
    QMenu menu;
    QAction *openAct = menu.addAction(QObject::tr("Abrir Predator Sense"));
    QObject::connect(openAct, &QAction::triggered, showWindow);
    menu.addSeparator();
    QActionGroup modes(&menu);
    auto rebuildModes = [&] {
        for (QAction *a : modes.actions()) {
            menu.removeAction(a);
            delete a;
        }
        const QVariantList list = backend.info().value(QStringLiteral("modes")).toList();
        const int current = backend.state().value(QStringLiteral("mode")).toInt();
        const bool ac = backend.state().value(QStringLiteral("onAC")).toBool();
        QAction *before = menu.actions().value(2);
        for (const QVariant &v : list) {
            const int m = v.toMap().value(QStringLiteral("value")).toInt();
            auto *a = new QAction(Backend::modeName(m), &modes);
            a->setCheckable(true);
            a->setChecked(m == current);
            a->setEnabled(ac || m == 1 || m == 6);
            QObject::connect(a, &QAction::triggered, &backend, [&backend, m] { backend.setMode(m); });
            menu.insertAction(before, a);
        }
    };
    menu.addSeparator();
    QAction *quitAct = menu.addAction(QObject::tr("Sair"));
    QObject::connect(quitAct, &QAction::triggered, &app, &QCoreApplication::quit);
    QObject::connect(&backend, &Backend::stateChanged, &menu, rebuildModes);
    QObject::connect(&backend, &Backend::infoChanged, &menu, rebuildModes);
    rebuildModes();
    tray.setContextMenu(&menu);
    tray.setToolTip(QStringLiteral("Predator Sense"));
    QObject::connect(&tray, &QSystemTrayIcon::activated, [&](QSystemTrayIcon::ActivationReason r) {
        if (r == QSystemTrayIcon::Trigger)
            window->isVisible() ? window->hide() : showWindow();
    });
    QObject::connect(&backend, &Backend::stateChanged, &tray, [&] {
        tray.setToolTip(QStringLiteral("Predator Sense — ") +
                        Backend::modeName(backend.state().value(QStringLiteral("mode")).toInt()));
    });
    if (trayAvailable)
        tray.show();
    else
        app.setQuitOnLastWindowClosed(true);

    // Development aid: PREDATOR_SENSE_SCREENSHOT=/path/prefix renders every page to PNG and exits.
    if (const QByteArray shot = qgetenv("PREDATOR_SENSE_SCREENSHOT"); !shot.isEmpty()) {
        window->show();
        auto *timer = new QTimer(&app);
        auto page = std::make_shared<int>(-1);
        QObject::connect(timer, &QTimer::timeout, &app, [=] {
            if (*page >= 0)
                window->grabWindow().save(QString::fromLocal8Bit(shot) + QString::number(*page) + QStringLiteral(".png"));
            if (++*page > 4) {
                QCoreApplication::quit();
                return;
            }
            window->setProperty("page", *page);
        });
        timer->start(2500);
    }

    return app.exec();
}

#include "main.moc"
