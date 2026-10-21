#include "Backend.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusVariant>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>

static QDBusConnection predatorBus()
{
    // PREDATOR_SENSE_BUS=session talks to "predator-sensed --simulate"
    return qgetenv("PREDATOR_SENSE_BUS") == "session" ? QDBusConnection::sessionBus() : QDBusConnection::systemBus();
}

static const auto kService = QStringLiteral("org.predatorsense.Daemon");
static const auto kPath = QStringLiteral("/org/predatorsense/Daemon");
static const auto kIface = QStringLiteral("org.predatorsense.Daemon1");

static QVariantMap parse(const QString &json)
{
    return QJsonDocument::fromJson(json.toUtf8()).object().toVariantMap();
}

static QString toJson(const QVariantMap &m)
{
    return QString::fromUtf8(QJsonDocument(QJsonObject::fromVariantMap(m)).toJson(QJsonDocument::Compact));
}

Backend::Backend(QObject *parent) : QObject(parent)
{
    m_sensorTimer.setInterval(1500);
    connect(&m_sensorTimer, &QTimer::timeout, this, &Backend::pollSensors);
    m_reconnectTimer.setInterval(3000);
    connect(&m_reconnectTimer, &QTimer::timeout, this, &Backend::reconnect);

    predatorBus().connect(kService, kPath, kIface, QStringLiteral("StateChanged"), this,
                                         SLOT(onStateChanged(QString)));
    reconnect();
}

QString Backend::modeName(int mode)
{
    switch (mode) {
    case 0: return tr("Silencioso");
    case 1: return tr("Equilibrado");
    case 4: return tr("Desempenho");
    case 5: return tr("Turbo");
    case 6: return tr("Eco");
    default: return tr("Desconhecido");
    }
}

void Backend::reconnect()
{
    m_iface = std::make_unique<QDBusInterface>(kService, kPath, kIface, predatorBus());
    m_iface->setTimeout(130000); // polkit may ask for a password
    QDBusReply<QString> info = m_iface->call(QStringLiteral("GetInfo"));
    const bool ok = info.isValid();
    if (ok) {
        m_info = parse(info.value());
        Q_EMIT infoChanged();
        refresh();
        pollSensors();
        m_reconnectTimer.stop();
    } else if (!m_reconnectTimer.isActive()) {
        m_reconnectTimer.start();
    }
    if (ok != m_connected) {
        m_connected = ok;
        Q_EMIT connectedChanged();
    }
}

void Backend::refresh()
{
    if (!m_iface)
        return;
    auto *w = new QDBusPendingCallWatcher(m_iface->asyncCall(QStringLiteral("GetState")), this);
    connect(w, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *w) {
        QDBusPendingReply<QString> r = *w;
        w->deleteLater();
        if (r.isValid())
            onStateChanged(r.value());
    });
}

void Backend::onStateChanged(const QString &json)
{
    m_state = parse(json);
    Q_EMIT stateChanged();
}

void Backend::setMonitoring(bool on)
{
    if (on == m_sensorTimer.isActive())
        return;
    if (on) {
        m_sensorTimer.start();
        pollSensors();
    } else {
        m_sensorTimer.stop();
    }
    Q_EMIT monitoringChanged();
}

void Backend::pollSensors()
{
    if (!m_connected || !m_iface)
        return;
    auto *w = new QDBusPendingCallWatcher(m_iface->asyncCall(QStringLiteral("GetSensors")), this);
    connect(w, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *w) {
        QDBusPendingReply<QString> r = *w;
        w->deleteLater();
        if (r.isValid()) {
            m_sensors = parse(r.value());
            Q_EMIT sensorsChanged();
        } else if (r.error().type() == QDBusError::ServiceUnknown || r.error().type() == QDBusError::NoReply) {
            m_connected = false;
            Q_EMIT connectedChanged();
            m_reconnectTimer.start();
        }
    });
}

void Backend::callAsync(const QString &method, const QVariantList &args)
{
    if (!m_iface) {
        Q_EMIT errorOccurred(tr("The Predator Sense service is not running."));
        return;
    }
    ++m_pending;
    Q_EMIT busyChanged();
    auto *w = new QDBusPendingCallWatcher(m_iface->asyncCallWithArgumentList(method, args), this);
    connect(w, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *w) {
        QDBusPendingReply<> r = *w;
        w->deleteLater();
        --m_pending;
        Q_EMIT busyChanged();
        if (r.isError()) {
            Q_EMIT errorOccurred(r.error().message());
            refresh(); // re-sync the UI with the real state
        }
    });
}

void Backend::setMode(int mode) { callAsync(QStringLiteral("SetOperatingMode"), {mode}); }
void Backend::setBatteryMode(int mode) { callAsync(QStringLiteral("SetBatteryMode"), {mode}); }
void Backend::setFan(const QVariantMap &fan) { callAsync(QStringLiteral("SetFanControl"), {toJson(fan)}); }
void Backend::setLighting(const QVariantMap &l) { callAsync(QStringLiteral("SetKeyboardLighting"), {toJson(l)}); }

void Backend::setOption(const QString &key, const QVariant &value)
{
    callAsync(QStringLiteral("SetOption"), {key, QVariant::fromValue(QDBusVariant(value))});
}

bool Backend::closeToTray() const { return QSettings().value(QStringLiteral("ui/closeToTray"), true).toBool(); }
bool Backend::fahrenheit() const { return QSettings().value(QStringLiteral("ui/fahrenheit"), false).toBool(); }

void Backend::setCloseToTray(bool on)
{
    QSettings().setValue(QStringLiteral("ui/closeToTray"), on);
    Q_EMIT prefsChanged();
}

void Backend::setFahrenheit(bool on)
{
    QSettings().setValue(QStringLiteral("ui/fahrenheit"), on);
    Q_EMIT prefsChanged();
}
