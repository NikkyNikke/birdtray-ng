#!/bin/sh
# Remove the Birdtray Wayland helper and glue files installed by
# install.sh.
#
# This script never touches Thunderbird profiles or mail data. To remove
# the add-on itself, use Thunderbird's Add-ons Manager.
set -eu

OPT="$HOME/.local/opt/birdtray-wayland"
NMH="$HOME/.mozilla/native-messaging-hosts"
AUTOSTART="$HOME/.config/autostart"

if pgrep -x birdtray >/dev/null 2>&1; then
    echo "A birdtray helper process is still running." >&2
    echo "Quit it via its tray menu (Quit Birdtray) first." >&2
    exit 1
fi

rm -f "$OPT/birdtray" "$OPT/launch-birdtray" "$OPT/native-messaging-host"
rm -f "$NMH/com.ulduzsoft.birdtray.json"
rm -f "$AUTOSTART/birdtray-wayland.desktop"
rmdir "$OPT" 2>/dev/null || true

echo "Removed helper and glue files."
echo "If the Thunderbird add-on is installed, remove it via Thunderbird's"
echo "Add-ons Manager (never delete profile files by hand)."
