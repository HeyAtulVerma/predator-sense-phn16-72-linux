#!/bin/bash
# Remove a Predator Sense installation made with tools/install.sh.
# (Arch package installs are removed with: sudo pacman -R predator-sense-linux)
set -euo pipefail
if [ "$(id -u)" -eq 0 ]; then SUDO=""; else SUDO="sudo"; fi
MANIFEST=/var/lib/predator-sense/install_manifest.txt

if command -v pacman >/dev/null && pacman -Qq predator-sense-linux >/dev/null 2>&1; then
    echo "Installed as a pacman package; run: sudo pacman -R predator-sense-linux"
    exit 1
fi

if command -v systemctl >/dev/null && [ -d /run/systemd/system ]; then
    $SUDO systemctl disable --now predator-sensed.service 2>/dev/null || true
fi
$SUDO modprobe -r predator_wmi 2>/dev/null || true
if command -v dkms >/dev/null; then
    for v in $($SUDO dkms status predator-wmi 2>/dev/null | sed -n 's#^predator-wmi/\([^,:]*\).*#\1#p' | sort -u); do
        $SUDO dkms remove "predator-wmi/$v" --all || true
        $SUDO rm -rf "/usr/src/predator-wmi-$v"
    done
fi
if [ -f "$MANIFEST" ]; then
    while IFS= read -r f; do
        [ -n "$f" ] && $SUDO rm -f -- "$f"
    done < "$MANIFEST"
fi
$SUDO rm -f /etc/modules-load.d/predator-wmi.conf /etc/xdg/autostart/predator-sense.desktop "$MANIFEST"
$SUDO rmdir --ignore-fail-on-non-empty /usr/share/predator-sense/models /usr/share/predator-sense 2>/dev/null || true
command -v systemctl >/dev/null && $SUDO systemctl daemon-reload 2>/dev/null || true
echo "Predator Sense removed. Settings are kept in /var/lib/predator-sense (delete it to reset)."
