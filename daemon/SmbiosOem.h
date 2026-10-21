#pragma once

#include <QByteArray>
#include <QString>

// Acer OEM SMBIOS tables, parsed the way AcerHardwareService does it.
//
// Type 172 (0xAC) "keyboard/lighting capabilities": 6-byte header (byte 4 =
// table type, byte 5 = table version) followed by 3-byte entries {function u8, value u16}:
//   0x08 KeyBoardArea  : 0 = per-key (USB), N = number of EC zones
//   0x0A KeyBoardColor : 1 = single color, 2 = RGB
//   0x17 Keyboard_Type : 1 = EC controlled, 2 = USB controlled
//
// Type 170 (0xAA) "hotkey functions": 14-byte header followed by 4-byte
// entries {function number u8, ?, type u16}. The keyboard backlight hotkey is
// the entry with 0x80 < number < 0xA0 and type 8 (Get_BK_Hotkey_Number).
struct SmbiosOem
{
    bool found172 = false;
    int keyboardArea = -1;   // -1 unknown
    int keyboardColor = -1;  // -1 unknown
    int keyboardInterface = -1; // fn 0x17: 1 EC, 2 USB, -1 unknown
    int tableType = -1;      // byte 4 of type 172
    int tableVersion = -1;   // byte 5 of type 172
    int bkHotkey = -1;       // -1 none

    static SmbiosOem read(const QString &dmiRoot = QStringLiteral("/sys/firmware/dmi/entries"));
    void parse172(const QByteArray &raw);
    void parse170(const QByteArray &raw);
};
