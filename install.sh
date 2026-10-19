#!/bin/sh
# Install the Birdtray Wayland helper and its glue files for the current
# user, from this repository's build output and deployment templates.
#
# This script never touches Thunderbird profiles or mail data.
set -eu

usage() {
    echo "usage: $0 [--bindir DIR] [--force]" >&2
    echo "  --bindir DIR  directory containing the built 'birdtray' binary" >&2
    echo "                (default: ./build)" >&2
    echo "  --force       replace an already-installed helper even if its" >&2
    echo "                hash differs" >&2
    exit 2
}

BINDIR=""
FORCE=0
while [ $# -gt 0 ]; do
    case "$1" in
        --bindir) [ $# -ge 2 ] || usage; BINDIR=$2; shift 2 ;;
        --force) FORCE=1; shift ;;
        *) usage ;;
    esac
done

REPO_ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
DEPLOY="$REPO_ROOT/deployment"

if [ -n "$BINDIR" ]; then
    BIN="$BINDIR/birdtray"
else
    BIN="$REPO_ROOT/build/birdtray"
fi
if [ ! -x "$BIN" ]; then
    echo "Helper binary not found at: $BIN" >&2
    echo "Build it first (see README):" >&2
    echo "  cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build" >&2
    exit 1
fi

OPT="$HOME/.local/opt/birdtray-wayland"
NMH="$HOME/.mozilla/native-messaging-hosts"
AUTOSTART="$HOME/.config/autostart"

expand() {
    # Expand a deployment template: @HOME@ -> invoking user's home.
    sed "s|@HOME@|$HOME|g" "$1"
}

install_file() { # template destination mode
    mkdir -p "$(dirname -- "$2")"
    expand "$1" > "$2"
    chmod "$3" "$2"
}

echo "Installing Birdtray Wayland for user $(id -un) (HOME=$HOME)"

mkdir -p "$OPT"

# 1) Helper binary, hash-guarded replacement.
if [ -e "$OPT/birdtray" ]; then
    OLD=$(sha256sum < "$OPT/birdtray" | cut -d' ' -f1)
    NEW=$(sha256sum < "$BIN" | cut -d' ' -f1)
    if [ "$OLD" != "$NEW" ] && [ "$FORCE" -ne 1 ]; then
        echo "An existing helper with a different hash is already installed:" >&2
        echo "  installed: $OLD" >&2
        echo "  new:       $NEW" >&2
        echo "Re-run with --force to replace it (quit the running helper via its" >&2
        echo "tray menu first)." >&2
        exit 1
    fi
fi
install -m 755 "$BIN" "$OPT/birdtray"

# 2) Glue files from templates.
install_file "$DEPLOY/launch-birdtray.template" "$OPT/launch-birdtray" 755
install_file "$DEPLOY/native-messaging-host.template" "$OPT/native-messaging-host" 755
install_file "$DEPLOY/com.ulduzsoft.birdtray.json.template" \
    "$NMH/com.ulduzsoft.birdtray.json" 644
install_file "$DEPLOY/birdtray-wayland.desktop.template" \
    "$AUTOSTART/birdtray-wayland.desktop" 644

echo
echo "Installed files (sha256):"
sha256sum "$OPT/birdtray" "$OPT/launch-birdtray" "$OPT/native-messaging-host" \
    "$NMH/com.ulduzsoft.birdtray.json" "$AUTOSTART/birdtray-wayland.desktop" \
    | sed "s|$HOME|~|g"

if pgrep -x birdtray >/dev/null 2>&1; then
    echo
    echo "NOTE: a birdtray helper process is already running."
    echo "Quit it via its tray menu (Quit Birdtray), then start the new one:"
else
    echo
    echo "Start the helper with:"
fi
echo "  $OPT/launch-birdtray &"
echo
echo "Then install the Thunderbird add-on (the XPI from Releases, or"
echo "artifacts/) via Thunderbird's Add-ons Manager > Install Add-on From File."
