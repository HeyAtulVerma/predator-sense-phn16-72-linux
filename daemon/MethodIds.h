#pragma once
// WmiMethodId of each method as declared by the MOF embedded in the firmware
// (WMI BMOF, object 05901221-D566-11D1-B2F0-00A0C9062910). Windows resolves the
// methods by name through this MOF; the IDs were read from the PHN16-72 BIOS
// V1.20 BMOF and can be re-checked on any machine with tools/bmof-verify.py.

namespace Mid {
// AcerGamingFunction (7A4DDFE7-5B5D-40B4-8595-4408E0CC7F56)
constexpr unsigned SetGamingProfile     = 1;
constexpr unsigned SetGamingLED         = 2;
constexpr unsigned GetGamingProfile     = 3;
constexpr unsigned GetGamingLED         = 4;
constexpr unsigned GetGamingSysInfo     = 5;
constexpr unsigned SetGamingRgbKb       = 6;
constexpr unsigned GetGamingRgbKb       = 7;
constexpr unsigned SetGamingFanBehavior = 14;
constexpr unsigned GetGamingFanBehavior = 15;
constexpr unsigned SetGamingFanSpeed    = 16;
constexpr unsigned GetGamingFanSpeed    = 17;
constexpr unsigned SetGamingKBBacklight = 20;
constexpr unsigned GetGamingKBBacklight = 21;
constexpr unsigned SetGamingMiscSetting = 22;
constexpr unsigned GetGamingMiscSetting = 23;

// APGeAction (61EF69EA-865C-4BC3-A502-A0DEBA0CB531)
constexpr unsigned SetFunction = 1;
constexpr unsigned GetFunction = 2;

// BatteryControl (79772EC5-04B1-4BFD-843C-61E7F77B6CC9)
constexpr unsigned GetBatteryHealthControlStatus = 20;
constexpr unsigned SetBatteryHealthControl       = 21;
} // namespace Mid
