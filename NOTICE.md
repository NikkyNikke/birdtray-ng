# NOTICE

This repository combines components with different origins and licences.
Each keeps its own licence and attribution.

## 1. The tray helper (`src/`, `deployment/`, `install.sh`, `uninstall.sh`, `tests/`)

Licence: **GPL-3.0-or-later** (see `LICENSE.txt`).

Lineage:

* **birdtray** — George Yunaev ([gyunaev/birdtray](https://github.com/gyunaev/birdtray)),
  the original system-tray mail notifier.
* **birdtray-wayland** — stark1tty's fork
  ([stark1tty/birdtray-wayland](https://github.com/stark1tty/birdtray-wayland)),
  adding Wayland and Flatpak Thunderbird support. This repository's helper is
  based on that fork's `master` (revision `2ac2016`, 2026-04-15) with the
  changes listed in the README's "What this fork changes" table, recorded in
  `patches/`.

## 2. The Thunderbird add-on (`extension/`)

Licence: **MPL-2.0** (see `extension/LICENSE`).

* **Original:** *Minimize on Close* by **rsjtdrjgfuzkfg (Dirk Steinmetz)**,
  [upstream repository](https://github.com/rsjtdrjgfuzkfg/thunderbird-minimizeonclose),
  add-on id `minimizeonclose@rsjtdrjgfuzkfg.com`, version 2.0.3.0.
* **This fork:** version 2.0.3.2, add-on id `birdtray-close@nikky.st`,
  compatibility 128.0–158.\*. The manifest keeps `author: rsjtdrjgfuzkfg`;
  this fork's modifications are MPL-2.0 as well.

File-by-file delta against the original:

| File | Status |
| --- | --- |
| `experiment.json`, `icon.png`, `LICENSE` | byte-identical to the original (`extension/LICENSE` SHA-256 `393eaf2b92b3d49bea139e39b6fd205b6e99c499999986aec5554586a830dfdd`, matching the original's `LICENSE`) |
| `background.js` | **substantially modified** (MPL-2.0 §3.4/Exhibit B notice in the file header): Birdtray native-messaging close-request integration with the original minimize fallback preserved, an `ensure-running` startup request, a duplicate window-registration guard, labeled diagnostic logging |
| `experiment.js` | **lightly modified** (notice in the file header): labeled diagnostic logging only; the window-lock API logic is unchanged |
| `manifest.json` | **modified**: fork id, description credit, version, range 128.0–158.\*, `nativeMessaging` permission |

The XPI in `artifacts/` is built from `extension/` by
`scripts/build_public_xpi.py`.

## 3. Signed add-ons in Thunderbird

Mozilla does not sign community add-ons, and Thunderbird 158 does not verify
signatures for sideloaded XPIs. This project therefore **does not** ask users
to disable `xpinstall` security preferences, and none are modified by the
add-on. Self-made signature files would provide no additional assurance and
are not used.

## 4. Trademarks

Thunderbird is a trademark of the Mozilla Foundation. Birdtray's name refers
to the original project by George Yunaev. This repository is not affiliated
with, or endorsed by, Mozilla or the original authors.
