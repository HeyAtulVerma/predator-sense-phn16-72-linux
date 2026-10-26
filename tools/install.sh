#!/bin/bash
# Predator Sense for Linux - installer for Arch, Fedora, Debian/Ubuntu and openSUSE families.
#
#   tools/install.sh              install dependencies, build and install everything
#   tools/install.sh --deps-only  only install build dependencies (used by CI)
#   tools/install.sh --no-kernel  skip kernel headers and the DKMS module
#   tools/install.sh --yes        do not ask before installing packages
#
# Run it as your normal user: it uses sudo only for the steps that need root.
set -euo pipefail

cd "$(dirname "$(readlink -f "$0")")/.."
SRC="$PWD"
VERSION="$(sed -n 's/^project([^ ]* VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)"
DEPS_ONLY=0 NO_KERNEL=0 YES=0
for a in "$@"; do
    case "$a" in
        --deps-only) DEPS_ONLY=1 ;;
        --no-kernel) NO_KERNEL=1 ;;
        --yes|-y) YES=1 ;;
        -h|--help) sed -n '2,10p' "$0"; exit 0 ;;
        *) echo "unknown option: $a" >&2; exit 2 ;;
    esac
done

if [ "$(id -u)" -eq 0 ]; then SUDO=""; else SUDO="sudo"; fi
say() { printf '\033[1;36m==>\033[0m %s\n' "$*"; }
warn() { printf '\033[1;33m!!\033[0m %s\n' "$*" >&2; }

# ------------------------------------------------------------------ distribution
# shellcheck source=/dev/null
. /etc/os-release
VERSION="$(sed -n 's/^project([^ ]* VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)"
FAMILY=""
for id in $ID ${ID_LIKE:-}; do
    case "$id" in
        arch|cachyos|manjaro|endeavouros|garuda) FAMILY=arch; break ;;
        fedora|rhel|centos|nobara|ultramarine) FAMILY=fedora; break ;;
        debian|ubuntu|linuxmint|pop|elementary|zorin|neon|kali) FAMILY=debian; break ;;
        opensuse*|suse|sles) FAMILY=suse; break ;;
    esac
done
[ -n "$FAMILY" ] || { warn "unsupported distribution '$ID'; install CMake, Qt >= 6.4 (Base, Declarative, Svg), DKMS and kernel headers manually, then use the manual steps in README.md"; exit 1; }
say "Distribution: ${PRETTY_NAME:-$ID} ($FAMILY family), Predator Sense $VERSION"

KREL="$(uname -r)"
case "$FAMILY" in
    arch)
        PKGS="base-devel cmake ninja pkgconf qt6-base qt6-declarative qt6-svg dbus polkit python"
        [ $NO_KERNEL -eq 1 ] || PKGS="$PKGS dkms"
        INSTALL="pacman -S --needed"; [ $YES -eq 1 ] && INSTALL="$INSTALL --noconfirm" ;;
    fedora)
        PKGS="gcc-c++ cmake ninja-build pkgconf-pkg-config systemd-devel qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qtsvg-devel dbus-daemon polkit python3"
        [ $NO_KERNEL -eq 1 ] || PKGS="$PKGS dkms kernel-devel-$KREL"
        INSTALL="dnf install"; [ $YES -eq 1 ] && INSTALL="$INSTALL -y" ;;
    debian)
        PKGS="build-essential cmake ninja-build pkg-config qt6-base-dev qt6-declarative-dev qt6-declarative-dev-tools qt6-svg-dev
              qml6-module-qtquick qml6-module-qtquick-controls qml6-module-qtquick-layouts qml6-module-qtquick-shapes
              qml6-module-qtquick-templates qml6-module-qtquick-window qml6-module-qtqml-workerscript
              dbus polkitd python3"
        # systemd.pc lives in systemd-dev on newer releases
        if apt-cache show systemd-dev >/dev/null 2>&1; then PKGS="$PKGS systemd-dev"; fi
        [ $NO_KERNEL -eq 1 ] || PKGS="$PKGS dkms linux-headers-$KREL"
        INSTALL="apt-get install"; [ $YES -eq 1 ] && INSTALL="env DEBIAN_FRONTEND=noninteractive apt-get install -y" ;;
    suse)
        PKGS="gcc-c++ cmake ninja pkgconf systemd-devel qt6-core-devel qt6-gui-devel qt6-widgets-devel qt6-dbus-devel
              qt6-qml-devel qt6-quick-devel qt6-quickcontrols2-devel qt6-svg-devel qt6-declarative-tools qt6-declarative-imports
              dbus-1 polkit python3"
        [ $NO_KERNEL -eq 1 ] || PKGS="$PKGS dkms kernel-devel kernel-default-devel"
        INSTALL="zypper install"; [ $YES -eq 1 ] && INSTALL="zypper --non-interactive install" ;;
esac

# On Arch the package manager should own the files: build a real package instead.
if [ "$FAMILY" = arch ] && [ $DEPS_ONLY -eq 0 ] && [ $NO_KERNEL -eq 0 ]; then
    say "Arch-based system: building and installing a pacman package (packaging/arch)"
    cd packaging/arch
    if [ $YES -eq 1 ]; then exec makepkg -si --noconfirm; else exec makepkg -si; fi
fi

say "Packages: $(echo "$PKGS" | tr -s ' \n' ' ')"
if [ $YES -eq 0 ]; then read -rp "Install them now? [Y/n] " ans; [[ "${ans:-y}" =~ ^[YySs]$ ]] || exit 1; fi
[ "$FAMILY" = debian ] && $SUDO apt-get update -qq
# shellcheck disable=SC2086
$SUDO $INSTALL $PKGS
[ $DEPS_ONLY -eq 1 ] && { say "Dependencies installed."; exit 0; }

# ------------------------------------------------------------------ build
say "Building"
# Clean stale CMake cache if source directory was moved or copied
if [ -f "$SRC/build-release/CMakeCache.txt" ]; then
    if ! grep -q "CMAKE_CACHEFILE_DIR:INTERNAL=$SRC/build-release" "$SRC/build-release/CMakeCache.txt" 2>/dev/null || \
       ! grep -q "CMAKE_HOME_DIRECTORY:INTERNAL=$SRC" "$SRC/build-release/CMakeCache.txt" 2>/dev/null; then
        say "Detected moved source or build directory; clearing stale build-release cache"
        rm -rf "$SRC/build-release"
    fi
fi

# Check power source and advise user
if [ -d /sys/class/power_supply ]; then
    ON_AC=0
    for ps in /sys/class/power_supply/*; do
        if [ -f "$ps/online" ] && [ "$(cat "$ps/online" 2>/dev/null)" = "1" ]; then
            ON_AC=1
            break
        fi
    done
    if [ $ON_AC -eq 0 ] && ls /sys/class/power_supply/BAT* >/dev/null 2>&1; then
        printf '\033[1;31m!! [BATTERY WARNING]\033[0m Laptop is running on battery. Please connect the AC charger to unlock Turbo/Performance modes.\n'
    fi
fi

cmake -S "$SRC" -B "$SRC/build-release" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr \
      -DBUILD_TESTING=ON >/dev/null
cmake --build "$SRC/build-release"
ctest --test-dir "$SRC/build-release" --output-on-failure

# ------------------------------------------------------------------ install
say "Installing files"
$SUDO cmake --install "$SRC/build-release"
$SUDO install -Dm644 "$SRC/build-release/install_manifest.txt" /var/lib/predator-sense/install_manifest.txt
$SUDO install -Dm644 "$SRC/data/predator-sense-autostart.desktop" /etc/xdg/autostart/predator-sense.desktop

if [ $NO_KERNEL -eq 0 ]; then
    say "Registering the predator_wmi kernel module with DKMS"
    DST="/usr/src/predator-wmi-$VERSION"
    if $SUDO dkms status predator-wmi 2>/dev/null | grep -q .; then
        for v in $($SUDO dkms status predator-wmi | sed -n 's#^predator-wmi/\([^,:]*\).*#\1#p' | sort -u); do
            $SUDO dkms remove "predator-wmi/$v" --all || true
        done
    fi
    $SUDO rm -rf "$DST"
    $SUDO install -d "$DST"
    $SUDO install -m644 "$SRC"/kernel/{predator_wmi.c,predator_wmi_uapi.h,Makefile,dkms.conf} "$DST"/
    $SUDO sed -i "s/^PACKAGE_VERSION=.*/PACKAGE_VERSION=\"$VERSION\"/" "$DST/dkms.conf"
    $SUDO dkms install "predator-wmi/$VERSION"
    echo predator_wmi | $SUDO tee /etc/modules-load.d/predator-wmi.conf >/dev/null

    if command -v mokutil >/dev/null && mokutil --sb-state 2>/dev/null | grep -qi enabled; then
        warn "Secure Boot is enabled: the module must be signed with a key enrolled in MOK."
        warn "Ubuntu/Debian DKMS signs it automatically after you enroll the key it offers (reboot to finish)."
        warn "On Fedora/openSUSE see the DKMS signing documentation, or the module will not load."
    fi
    $SUDO modprobe predator_wmi || warn "predator_wmi did not load now (see 'sudo dmesg')"
fi

if grep -rqs '^\s*blacklist\s\+acer_wmi' /etc/modprobe.d/; then
    warn "acer_wmi is blacklisted in /etc/modprobe.d: the Predator mode key and hotkeys need it."
fi

if command -v systemctl >/dev/null && [ -d /run/systemd/system ]; then
    say "Enabling and starting the service"
    $SUDO systemctl daemon-reload
    $SUDO systemctl reload dbus 2>/dev/null || true
    $SUDO systemctl enable predator-sensed.service
    $SUDO systemctl restart predator-sensed.service
else
    warn "systemd not detected: predator-sensed starts on demand through D-Bus activation."
fi
say "Done. Open \"Predator Sense\" from the application menu or run: predatorctl status"
