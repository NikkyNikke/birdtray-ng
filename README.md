# Birdtray — close-to-tray for Thunderbird on KDE Plasma Wayland

> **This is a personal fork of [stark1tty/birdtray-wayland](https://github.com/stark1tty/birdtray-wayland)**,
> which is itself a fork of [gyunaev/birdtray](https://github.com/gyunaev/birdtray).
> This fork targets a **natively installed Thunderbird (128–158) on KDE Plasma Wayland**
> and adds a *real* close-to-tray experience: clicking the message window's ✕ hides
> it to the tray instead of exiting Thunderbird — with the tray icon bringing it back —
> while the natural ways of quitting Thunderbird keep working.
>
> The upstream project targets X11 (and, in stark1tty's fork, Flatpak Thunderbird on
> Wayland). See [What this fork changes](#what-this-fork-changes) for exactly what was
> added on top, and [Known limitations](#known-limitations) for what has *not* been
> demonstrated.

Birdtray upstream: *a system tray new mail notification for Thunderbird, which does
not require extensions.*

## What this fork changes

All helper changes are recorded as reviewable patch files under
[`patches/`](patches/) and as a short stack of commits on top of upstream
`master` (revision `2ac2016`, 2026-04-15):

| Change | Why | Patch |
| --- | --- | --- |
| **Native messaging host mode** in the helper + per-user control socket | lets a Thunderbird add-on talk to the tray helper | `patches/helper-delta.patch` |
| **One-shot quit authorization** | an ✕ click may only *exit* Thunderbird when a human explicitly chose *Quit Thunderbird* from the tray menu; this removed upstream birdtray's stale 15-second "allow-close" window as the only gate | `patches/helper-delta.patch` |
| **`skipSwitcher` added to the tray-hide script** | a tray-hidden window also disappears from **Alt+Tab** and from the **Meta+W** overview, not just from the taskbar; restore clears all four flags | `patches/skip-switcher.patch` |
| Helper auto-start (on demand, one-shot spawn) | Thunderbird starting without the helper can request it over native messaging; the helper then exists for tray interactions | `patches/helper-delta.patch` |

The two patch files are derived directly from the commits and are the
complete reviewable delta: `patches/helper-delta.patch` is the full helper
change against upstream `master` (revision `2ac2016`), and
`patches/skip-switcher.patch` is the incremental delta applying on top of
it. Applying both, in that order, to a pristine upstream checkout
reproduces the helper code committed here.
| **Thunderbird add-on** in `extension/` | the ✕ click needs to be intercepted inside Thunderbird — done by forking Dirk Steinmetz's maintained *Minimize on Close* add-on | see [The Thunderbird add-on](#the-thunderbird-add-on) |
| Deployment kit in `deployment/` + `install.sh` / `uninstall.sh` | reproducible per-user install with `$HOME` templates | n/a (new files) |
| New unit tests (14 in `ctest`) | protocol, authorization and window-state logic are testable without a GUI | `tests/` |

## How it works (plain English)

1. The **add-on** locks Thunderbird's main window so its ✕ cannot close it, and
   forwards the click to the **helper** over native messaging.
2. The helper asks **KWin** (via its scripting interface) to hide the window with
   *four* flags at once: `minimized`, `skipTaskbar`, `skipPager` and `skipSwitcher`.
   That is why a hidden Thunderbird is absent from the taskbar, Alt+Tab *and* the
   Meta+W overview. The helper replies `hidden` to the add-on.
3. Clicking the tray icon (or choosing *Show Thunderbird*) asks KWin to clear all
   four flags — the window reappears exactly as it was.
4. **Quit Thunderbird** in the tray menu grants a one-shot close authorization and
   closes the window; the add-on asks the helper again, gets `allow-close` this
   time, unlocks the window and lets Thunderbird exit for real.
5. **Quit Birdtray** in the tray menu first restores a hidden Thunderbird (it
   refuses to quit if the restore fails, so the window can never be lost), then
   exits the helper.
6. Clicking ✕ when *no* helper answers (e.g. the helper was never started) falls
   back to the original *Minimize on Close* behaviour: the window minimizes to the
   taskbar instead of exiting Thunderbird. Thunderbird still survives; you can
   restore it from the taskbar or tray.

Two distinct quits, two different results — that separation is the point.

## Components

| Component | Path | Language | Licence |
| --- | --- | --- | --- |
| Tray helper (`birdtray`) | `src/` | C++ / Qt 6 | GPL-3.0-or-later |
| Thunderbird add-on | `extension/` | JavaScript (WebExtension + experiment API) | MPL-2.0 |
| Deployment templates & installers | `deployment/`, `install.sh`, `uninstall.sh` | POSIX shell | GPL-3.0-or-later |
| Test suite | `tests/` | C++ (GTest/GMock) | GPL-3.0-or-later |

## Tested environment

Everything below was **actually exercised live**, repeatedly:

* CachyOS (Arch Linux), KDE Plasma on **Wayland**, KWin compositor
* Thunderbird **158.0.1**, natively installed (pacman), single profile
* Birdtray helper built with gcc + Qt 6
* The close → hide → tray-restore cycle, the minimize ⇄ taskbar behaviour,
  both quit paths, Alt+Tab/Meta+W exclusion in both directions, helper
  re-launch races, and a 14-test `ctest` suite.

If your setup differs (other distributions, other Wayland compositors, X11
sessions, Flatpak/Snap Thunderbird, multiple profiles), treat it as
**untested**: it may work, and patches are welcome, but nothing here claims
compatibility beyond the tested matrix.

## Known limitations

1. **Process detection by substring scan.** To answer "is Thunderbird running?",
   the helper scans `/proc/*/cmdline` for the configured process name
   (`advanced/tbprocessname`, default `thunderbird`). The match is a plain
   substring, so *any* process whose command line contains that word (say, an
   editor with a file named `thunderbird-notes.txt` open) can produce a false
   positive. Consequence: helper features that depend on the answer may act
   when Thunderbird is not actually running. Window-based actions (hide/show/
   quit) are unaffected in practice because they additionally require a real
   Thunderbird *window* found through KWin.
2. **Cold-start race (once observed, not reproduced).** If the helper starts up
   at the very moment Thunderbird is showing its first window, one ✕ click was
   observed to fall back to a plain *taskbar* minimize instead of tray-hide.
   Thunderbird was never at risk; a single tray click restored the window
   normally. Diagnosed, understood (single-shot fallback, no retry), and
   deliberately left unfixed until it ever recurs — a retry-around-hide fix is
   designed but gated on a real reproduction.
3. **KWin-specific.** Hiding/restoring uses KWin's DBus scripting interface
   (`qdbus6`). On GNOME or other compositors this will not work; nobody has
   tested it.
4. **Native Thunderbird only (tested).** This fork's kit was built and tested
   against a natively installed Thunderbird. Upstream stark1tty fork's Flatpak
   support is untouched but untested here.
5. **Unsigned add-on.** Mozilla does not sign community add-ons, and
   Thunderbird ≥ 128 (like this release) does not verify XPI signatures for
   sideloaded files — the add-on installs normally through the Add-ons Manager
   *without* weakening `xpinstall` security preferences. See the add-on section.
6. **No auto-update channel.** The XPI has no update URL; update by downloading
   a new release manually.
7. **Version range 128.0–158.\*.** Declared in the manifest; actually tested
   only on 158.0.1.

## Dependencies

**Build:** CMake ≥ 3.5, a C++ compiler (gcc/clang), Qt 6 modules
Core, Gui, Widgets, Network, DBus, Svg, and Qt6LinguistTools (`lrelease`).
GTest/GMock for the test suite.

**Runtime:** KDE Plasma (a StatusNotifierItem-capable system tray + KWin),
`qdbus6` (from `qt6-tools` on Arch), and for the add-on: Thunderbird 128–158.

## Build the helper

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_WITH_TESTS=ON
cmake --build build
```

(The helper itself builds fine without `-DBUILD_WITH_TESTS=ON`; the flag
adds the GTest/GMock unit-test targets and needs system GTest.)

Sanity-check the result:

```sh
ctest --test-dir build        # 14 tests
```

## Install (per user, no root)

```sh
./install.sh --bindir build
```

`install.sh` copies the helper to `~/.local/opt/birdtray-wayland/`, installs the
launcher, the native messaging host wrapper, the host registration JSON and the
KDE autostart entry — all from `$HOME`-templated files in `deployment/`. It
prints the SHA-256 of everything it installs, refuses to silently replace an
existing different helper (pass `--force` after quitting the old one), and never
touches Thunderbird profiles or mail data.

Uninstall is symmetric: `./uninstall.sh` (it refuses to run while a helper
process is alive).

The launcher gives the helper its **own isolated XDG state** under
`~/.local/share/birdtray-wayland/` (`config/`, `data/`, `cache/`) so its
settings never collide with other applications'. Notable settings there
(QSettings-style flat keys in `config/birdtray-config.json`):

* `common/startClosedThunderbird = true` — tray click launches Thunderbird when none is running
* `common/hideWhenStartedManually = false` — a manually started Thunderbird stays visible
* `advanced/tbprocessname = thunderbird` — see limitation 1

## The Thunderbird add-on

`extension/` contains the add-on, and `artifacts/` a built XPI:

`minimize-on-close-birdtray-wayland-158-2.0.3.2-public.xpi`
(SHA-256 `69b580b05b3792477663b44b359e19efb7e8c714829b8945c99c49a99c8e2fec`;
reproducible — `scripts/build_public_xpi.py` packs from `extension/` with fixed
timestamps).

Install it via Thunderbird's **Add-ons Manager → ⚙ → Install Add-on From File…**

* ID: `birdtray-close@nikky.st`, compatibility 128.0–158.\*
* It is a fork of Dirk Steinmetz's *Minimize on Close*
  ([upstream](https://github.com/rsjtdrjgfuzkfg/thunderbird-minimizeonclose),
  `minimizeonclose@rsjtdrjgfuzkfg.com`, MPL-2.0) — see [`NOTICE.md`](NOTICE.md)
  for the full provenance and the file-by-file delta. The author field credits
  the original author; this fork's changes are MPL-2.0 as well, with per-file
  modification notices.
* **Unsigned, on purpose.** Community add-ons like this one are not signed by
  Mozilla — a Thunderbird team member has stated plainly that Mozilla does not
  sign add-ons. Thunderbird 158 does not verify sideloaded XPI signatures, so
  the add-on installs normally; there is no need to disable signature
  enforcement (`xpinstall.*` stays untouched), and no such workaround is
  recommended by this project.
* The add-on's diagnostic logging (`[BIRDTRAY-WAYLAND] …` lines) is kept in the
  public build — it appears only in Thunderbird's developer-tools console and
  has proven its worth whenever something needed investigating.
* The experiment API (`ex_windows`) is what locks/unlocks the window; if you
  audit the code, that is the part to read (`experiment.js`), plus the close
  flow in `background.js`.

## Native messaging and single instance

* The host name `com.ulduzsoft.birdtray` maps (via
  `~/.mozilla/native-messaging-hosts/com.ulduzsoft.birdtray.json`, mode 0644)
  to the helper's stdio wrapper; the JSON allowlists exactly
  `birdtray-close@nikky.st`.
* Messages are length-prefixed (4-byte little-endian) on stdio; control
  requests (`ensure-running`, `close-request`) go over a per-user **abstract
  local socket** `birdtray-wayland-control-<uid>` that only the current user
  can reach.
* The add-on sends `ensure-running` at Thunderbird startup, so the helper is
  always there when a ✕ click needs it — if it is not, Thunderbird starts a
  fresh helper process on the spot (one-shot spawn), which then takes over
  tray duty.

## Uninstall / recovery / rollback

* **Uninstall:** `./uninstall.sh`, then remove the add-on via Thunderbird's
  Add-ons Manager. Your mail and profiles are never touched by anything here.
* **Recovery:** if the tray menu is unreachable, quit the helper process and
  simply start Thunderbird normally — worst case, closes behave like the
  upstream minimize-on-close add-on (taskbar, not tray).
* **Rollback per feature:** every helper delta lives in one patch under
  `patches/`; revert the feature by reverting the commit / patch and
  rebuilding.

## Licence and attribution

* Helper, deployment kit, tests: **GPL-3.0-or-later** (upstream birdtray by
  George Yunaev; Wayland/Flatpak patches by stark1tty; this fork's additions
  under the same licence).
* Thunderbird add-on: **MPL-2.0**, original work by rsjtdrjgfuzkfg (Dirk
  Steinmetz); this fork's modifications remain MPL-2.0 with notices in the
  modified files. Full lineage in [`NOTICE.md`](NOTICE.md).

Thunderbird is a trademark of the Mozilla Foundation. This project is not
affiliated with or endorsed by Mozilla.

No warranty. This is a personal fork published in the hope that it is useful;
see the licences for terms.
