#pragma once

#include <QDBusContext>
#include <QDBusMessage>
#include <QDBusVariant>
#include <QObject>
#include <functional>

class Daemon;

// System-bus API: org.predatorsense.Daemon1 at /org/predatorsense/Daemon.
// Reads are open to everyone; changes need the polkit action
// org.predatorsense.control (allowed for the active local session by default).
class DBusService : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.predatorsense.Daemon1")
public:
    explicit DBusService(Daemon *d, bool requireAuth = true);

public Q_SLOTS:
    QString GetInfo();
    QString GetState();
    QString GetSensors();
    void SetOperatingMode(int mode);
    void SetBatteryMode(int mode);
    void SetFanControl(const QString &json);
    void SetKeyboardLighting(const QString &json);
    void SetOption(const QString &key, const QDBusVariant &value);

Q_SIGNALS:
    void StateChanged(const QString &json);

private:
    void authorized(std::function<QString()> action);
    Daemon *m_d;
    bool m_requireAuth;
};
