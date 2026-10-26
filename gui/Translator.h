#pragma once

#include <QHash>
#include <QString>
#include <QTranslator>

class AppTranslator : public QTranslator
{
    Q_OBJECT
public:
    explicit AppTranslator(QObject *parent = nullptr);

    QString translate(const char *context, const char *sourceText,
                      const char *disambiguation = nullptr, int n = -1) const override;

    bool isActive() const { return m_active; }
    void setActive(bool active) { m_active = active; }

private:
    bool m_active = true;
    QHash<QString, QString> m_dict;
};
