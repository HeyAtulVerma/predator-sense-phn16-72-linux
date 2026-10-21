#pragma once

#include <QDBusInterface>
#include <QObject>
#include <QTimer>
#include <QVariantMap>
#include <memory>

// Bridge between QML and predator-sensed (system D-Bus).
class Backend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QVariantMap info READ info NOTIFY infoChanged)
    Q_PROPERTY(QVariantMap state READ state NOTIFY stateChanged)
    Q_PROPERTY(QVariantMap sensors READ sensors NOTIFY sensorsChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool monitoring READ monitoring WRITE setMonitoring NOTIFY monitoringChanged)
    // UI preferences (per user, ~/.config/predator-sense/predator-sense.conf)
    Q_PROPERTY(bool closeToTray READ closeToTray WRITE setCloseToTray NOTIFY prefsChanged)
    Q_PROPERTY(bool fahrenheit READ fahrenheit WRITE setFahrenheit NOTIFY prefsChanged)

public:
    explicit Backend(QObject *parent = nullptr);

    bool connected() const { return m_connected; }
    QVariantMap info() const { return m_info; }
    QVariantMap state() const { return m_state; }
    QVariantMap sensors() const { return m_sensors; }
    bool busy() const { return m_pending > 0; }
    bool monitoring() const { return m_sensorTimer.isActive(); }
    void setMonitoring(bool on);
    bool closeToTray() const;
    void setCloseToTray(bool on);
    bool fahrenheit() const;
    void setFahrenheit(bool on);

    Q_INVOKABLE void setMode(int mode);
    Q_INVOKABLE void setBatteryMode(int mode);
    Q_INVOKABLE void setFan(const QVariantMap &fan);
    Q_INVOKABLE void setLighting(const QVariantMap &lighting);
    Q_INVOKABLE void setOption(const QString &key, const QVariant &value);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE static QString modeName(int mode);

Q_SIGNALS:
    void connectedChanged();
    void infoChanged();
    void stateChanged();
    void sensorsChanged();
    void busyChanged();
    void monitoringChanged();
    void prefsChanged();
    void errorOccurred(const QString &message);

private Q_SLOTS:
    void onStateChanged(const QString &json);
    void pollSensors();
    void reconnect();

private:
    void callAsync(const QString &method, const QVariantList &args);
    std::unique_ptr<QDBusInterface> m_iface;
    bool m_connected = false;
    QVariantMap m_info, m_state, m_sensors;
    int m_pending = 0;
    QTimer m_sensorTimer;
    QTimer m_reconnectTimer;
};
