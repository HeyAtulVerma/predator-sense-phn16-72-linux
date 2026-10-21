# Changelog

All notable changes to this project are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Changed
- `dkms.conf` no longer sets the `CLEAN` option, which DKMS 3 reports as deprecated.
- WMI method IDs and parameter layouts are now confirmed against the firmware's own MOF (BMOF);
  `tools/bmof-verify.py` re-checks them on any machine.
- `GetGamingKBBacklight` and `SetBatteryHealthControl` results parsed with the layouts declared by the MOF.

### Added
- `tools/install.sh` / `tools/uninstall.sh` for Arch, Fedora, Debian/Ubuntu and openSUSE families; CI builds and
  tests on all of them, and builds the module against Arch, Ubuntu and Fedora kernels.
- Qt 6.4 support (Debian 12, Ubuntu 24.04), kernels older than 6.9, D-Bus activation without systemd.
- Static keyboard colours send the zone-mask `SetGamingLED` step used by AcerLightingService, with the buffer
  size chosen from the SMBIOS type 172 version.
- Unit test with the real PHN16-72 SMBIOS OEM tables.

## [0.9.0] - 2026-10-21

First public beta.

### Added
- `predator_wmi` kernel module: allow-listed pass-through to the AcerGamingFunction, APGeAction and BatteryControl
  WMI interfaces, DKMS packaging, clang-built kernel support.
- `predator-sensed` service re-implementing PredatorSense 5.0.1463: operating modes with AC/DC memory and battery
  lock, fan control (auto/max/custom per fan), 4-zone RGB keyboard (static zones and 7 effects), keyboard backlight
  timeout, boot sound, LCD overdrive, 80 % charge limit, battery calibration, power-off USB charging, EC sensors,
  Acer GPU clock profile per mode through NVML, restore after resume, `platform_profile` notifications.
- D-Bus API with polkit authorisation, `predatorctl` CLI.
- Qt 6 / QML application with tray icon.
- `--simulate` mode for development without hardware.
- Protocol documentation (`docs/REVERSE_ENGINEERING.md`).

[0.9.0]: https://github.com/aishend/predator-sense-phn16-72-linux/releases/tag/v0.9.0
