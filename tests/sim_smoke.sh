#!/bin/bash
# End-to-end test against the simulated firmware, on a private session bus.
# Usage: tests/sim_smoke.sh [build-dir]
set -euo pipefail
BUILD="$(realpath "${1:-build}")"
if [ -z "${PREDATOR_SMOKE_INNER:-}" ]; then
    exec env PREDATOR_SMOKE_INNER=1 dbus-run-session -- "$0" "$@"
fi
STATE="$(mktemp)"
rm -f "$STATE"
"$BUILD/predator-sensed" --simulate --state "$STATE" &
DPID=$!
trap 'kill $DPID 2>/dev/null; rm -f "$STATE"' EXIT
export PREDATOR_SENSE_BUS=session
CTL="$BUILD/predatorctl"
for _ in $(seq 50); do "$CTL" info >/dev/null 2>&1 && break; sleep 0.1; done

fail() { echo "FAIL: $*" >&2; exit 1; }
json() { python3 -c "import json,sys; d=json.load(sys.stdin); print($1)"; }

"$CTL" info | json 'len(d["modes"])' | grep -qx 5 || fail "mode capability"
"$CTL" mode turbo
[ "$("$CTL" status | json 'd["mode"]')" = 5 ] || fail "set turbo"
"$CTL" fan custom 70 60
[ "$("$CTL" status | json 'd["fan"]["cpu"]["speed"]')" = 70 ] || fail "custom fan"
"$CTL" mode quiet
[ "$("$CTL" status | json 'd["fan"]["mode"]')" = auto ] || fail "quiet must force auto fans"
"$CTL" fan max && fail "fan max must be refused in quiet"
"$CTL" kb static '#ff0000' '#00ff00' '#0000ff' '#ffffff'
[ "$("$CTL" status | json 'd["lighting"]["zones"][2]')" = '#0000ff' ] || fail "zones"
"$CTL" kb wave --speed 7 --direction left
"$CTL" set usbCharging on
"$CTL" set usbChargingThreshold 20
[ "$("$CTL" status | json 'd["usbCharging"], d["usbChargingThreshold"]')" = "True 20" ] || fail "usb charging"
"$CTL" set batteryLimit on
[ "$("$CTL" status | json 'd["batteryLimit"]')" = True ] || fail "battery limit"
"$CTL" set lcdOverdrive on && fail "lcd overdrive is unsupported in the simulator"
"$CTL" battery-mode eco
[ "$("$CTL" status | json 'd["dcMode"]')" = 6 ] || fail "battery mode"
"$CTL" sensors | json 'd["cpuTemp"] > 0' | grep -qx True || fail "sensors"
echo "smoke test passed"
