#include "SmbiosOem.h"

#include <QDir>
#include <QFile>

static QByteArray readRaw(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return {};
    return f.readAll();
}

SmbiosOem SmbiosOem::read(const QString &dmiRoot)
{
    SmbiosOem s;
    QDir dir(dmiRoot);
    for (const QString &e : dir.entryList({QStringLiteral("172-*")}, QDir::Dirs))
        s.parse172(readRaw(dir.filePath(e + QStringLiteral("/raw"))));
    for (const QString &e : dir.entryList({QStringLiteral("170-*")}, QDir::Dirs))
        s.parse170(readRaw(dir.filePath(e + QStringLiteral("/raw"))));
    return s;
}

void SmbiosOem::parse172(const QByteArray &raw)
{
    if (raw.size() < 6 || quint8(raw[0]) != 0xAC)
        return;
    const int len = quint8(raw[1]);
    if (len > raw.size())
        return;
    found172 = true;
    tableType = quint8(raw[4]);
    tableVersion = quint8(raw[5]);
    for (int off = 6; off + 3 <= len; off += 3) {
        const int fn = quint8(raw[off]);
        const int val = quint8(raw[off + 1]) | (quint8(raw[off + 2]) << 8);
        switch (fn) {
        case 0x08: keyboardArea = val; break;
        case 0x0A: keyboardColor = val; break;
        case 0x17: keyboardInterface = val; break;
        default: break;
        }
    }
}

void SmbiosOem::parse170(const QByteArray &raw)
{
    if (raw.size() < 14 || quint8(raw[0]) != 0xAA)
        return;
    const int len = quint8(raw[1]);
    if (len > raw.size())
        return;
    for (int off = 14; off + 4 <= len; off += 4) {
        const int number = quint8(raw[off]);
        const int type = quint8(raw[off + 2]) | (quint8(raw[off + 3]) << 8);
        if (number > 0x80 && number < 0xA0 && type == 8) {
            bkHotkey = number;
            return;
        }
    }
}
