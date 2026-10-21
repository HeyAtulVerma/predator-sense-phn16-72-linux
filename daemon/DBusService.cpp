#include "DBusService.h"
#include "Daemon.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMetaType>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QJsonDocument>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcBus, "predator.dbus", QtInfoMsg)

namespace {
struct PolkitSubject {
    QString kind;
    QVariantMap details;
};
struct PolkitResult {
    bool authorized = false;
    bool challenge = false;
    QMap<QString, QString> details;
};
} // namespace
Q_DECLARE_METATYPE(PolkitSubject)
Q_DECLARE_METATYPE(PolkitResult)

static QDBusArgument &operator<<(QDBusArgument &a, const PolkitSubject &s)
{
    a.beginStructure();
    a << s.kind << s.details;
    a.endStructure();
    return a;
}
static const QDBusArgument &operator>>(const QDBusArgument &a, PolkitSubject &s)
{
    a.beginStructure();
    a >> s.kind >> s.details;
    a.endStructure();
    return a;
}
static QDBusArgument &operator<<(QDBusArgument &a, const PolkitResult &r)
{
    a.beginStructure();
    a << r.authorized << r.challenge << r.details;
    a.endStructure();
    return a;
}
static const QDBusArgument &operator>>(const QDBusArgument &a, PolkitResult &r)
{
    a.beginStructure();
    a >> r.authorized >> r.challenge >> r.details;
    a.endStructure();
    return a;
}

static QString json(const QJsonObject &o)
{
    return QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact));
}

DBusService::DBusService(Daemon *d, bool requireAuth) : QObject(d), m_d(d), m_requireAuth(requireAuth)
{
    qDBusRegisterMetaType<PolkitSubject>();
    qDBusRegisterMetaType<PolkitResult>();
    qDBusRegisterMetaType<QMap<QString, QString>>();
    connect(d, &Daemon::stateChanged, this, [this] { Q_EMIT StateChanged(json(m_d->state())); });
}

QString DBusService::GetInfo() { return json(m_d->info()); }
QString DBusService::GetState() { return json(m_d->state()); }
QString DBusService::GetSensors() { return json(m_d->sensors()); }

void DBusService::authorized(std::function<QString()> action)
{
    setDelayedReply(true);
    const QDBusMessage msg = message();
    QDBusConnection bus = connection();

    auto finish = [msg, bus, action]() mutable {
        const QString err = action();
        if (err.isEmpty())
            bus.send(msg.createReply());
        else
            bus.send(msg.createErrorReply(QStringLiteral("org.predatorsense.Error.Failed"), err));
    };

    // root (e.g. "sudo predatorctl") does not need polkit
    if (!m_requireAuth || bus.interface()->serviceUid(msg.service()).value() == 0) {
        finish();
        return;
    }

    PolkitSubject subject{QStringLiteral("system-bus-name"), {{QStringLiteral("name"), msg.service()}}};
    QDBusMessage check = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.PolicyKit1"), QStringLiteral("/org/freedesktop/PolicyKit1/Authority"),
        QStringLiteral("org.freedesktop.PolicyKit1.Authority"), QStringLiteral("CheckAuthorization"));
    check << QVariant::fromValue(subject) << QStringLiteral("org.predatorsense.control")
          << QVariant::fromValue(QMap<QString, QString>()) << uint(1) /* AllowUserInteraction */ << QString();

    auto *w = new QDBusPendingCallWatcher(bus.asyncCall(check, 120000), this);
    connect(w, &QDBusPendingCallWatcher::finished, this, [msg, bus, finish](QDBusPendingCallWatcher *w) mutable {
        QDBusPendingReply<PolkitResult> r = *w;
        w->deleteLater();
        if (r.isError()) {
            qCWarning(lcBus) << "polkit error:" << r.error().message();
            bus.send(msg.createErrorReply(QStringLiteral("org.predatorsense.Error.Auth"), r.error().message()));
        } else if (!r.value().authorized) {
            bus.send(msg.createErrorReply(QStringLiteral("org.predatorsense.Error.NotAuthorized"),
                                          QStringLiteral("not authorized")));
        } else {
            finish();
        }
    });
}

void DBusService::SetOperatingMode(int mode)
{
    authorized([this, mode] { return m_d->setOperatingMode(mode); });
}

void DBusService::SetBatteryMode(int mode)
{
    authorized([this, mode] { return m_d->setBatteryMode(mode); });
}

void DBusService::SetFanControl(const QString &j)
{
    const QJsonObject o = QJsonDocument::fromJson(j.toUtf8()).object();
    authorized([this, o] { return m_d->setFanControl(o); });
}

void DBusService::SetKeyboardLighting(const QString &j)
{
    const QJsonObject o = QJsonDocument::fromJson(j.toUtf8()).object();
    authorized([this, o] { return m_d->setKeyboardLighting(o); });
}

void DBusService::SetOption(const QString &key, const QDBusVariant &value)
{
    const QVariant v = value.variant();
    authorized([this, key, v] { return m_d->setOption(key, v); });
}
