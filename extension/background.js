// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// Modified from the original "Minimize on Close" Thunderbird add-on
// (https://github.com/rsjtdrjgfuzkfg/thunderbird-minimizeonclose),
// Copyright (C) rsjtdrjgfuzkfg (Dirk Steinmetz), Mozilla Public License
// 2.0. Modifications by the Birdtray Wayland fork maintainers, also
// released under the Mozilla Public License 2.0: Birdtray
// native-messaging integration for close-to-tray (close-request to the
// helper with the original minimize fallback preserved), an
// ensure-running startup request so the tray helper starts with
// Thunderbird, a duplicate window-registration guard, and labeled
// diagnostic logging.
(async function(){
  const nativeHost = "com.ulduzsoft.birdtray";
  let mainWindows = []; // window ids of currently open main windows

  // Called whenever a main window is attempting to close, instead of actually
  // closing.
  const windowClosingCallback = async function(windowId) {
    console.warn("[BIRDTRAY-WAYLAND] background onClosing", windowId);
    let index = mainWindows.indexOf(windowId);
    if (index >= 0) {
      if (mainWindows.length <= 1 &&
          (await messenger.windows.get(windowId)).state != "minimized") {
        // Ask the tray helper to hide the window. If it is unavailable or KWin
        // fails, preserve the original minimize fallback.
        let response = null;
        try {
          console.warn("[BIRDTRAY-WAYLAND] sending native close-request", windowId);
          response = await messenger.runtime.sendNativeMessage(nativeHost, {
            type: "close-request",
            windowId
          });
          console.warn("[BIRDTRAY-WAYLAND] native response", response);
        } catch (error) {
          console.warn("Birdtray close-to-tray request failed:", error);
        }
        if (response && response.action === "hidden") {
          return;
        }
        if (!response || response.action !== "allow-close") {
          console.warn("[BIRDTRAY-WAYLAND] fallback minimize", response);
          await messenger.windows.update(windowId, { "state": "minimized" });
          return;
        }
        mainWindows.splice(index, 1);
      } else {
        // Permit closing non-last and minimized main windows
        mainWindows.splice(index, 1);
      }
    }
    // Permit closing. As we locked the window, we need to unlock it first.
    await messenger.ex_windows.unlockWindow(windowId);
    await messenger.windows.remove(windowId);
  };
  await messenger.ex_windows.onClosing.addListener(windowClosingCallback);
  console.warn("[BIRDTRAY-WAYLAND] background initialized");

  // Called for each window present while starting up, and any window opened
  // afterwards
  const windowRegistrationListener = async function(windowInfo) {
    if (windowInfo.id && windowInfo.type === "normal" &&
        !mainWindows.includes(windowInfo.id)) {
      mainWindows.push(windowInfo.id);
      console.warn("[BIRDTRAY-WAYLAND] registering normal window", windowInfo.id);
      await messenger.ex_windows.lockWindow(windowInfo.id);
    }
  }
  await messenger.windows.onCreated.addListener(windowRegistrationListener);
  const windowInfos = await messenger.windows.getAll()
  for (let windowInfo of windowInfos) {
    await windowRegistrationListener(windowInfo);
  }

  // Start Birdtray automatically when Thunderbird starts. The native host
  // only starts a helper if its per-user control socket is not already active.
  try {
    console.warn("[BIRDTRAY-WAYLAND] ensuring tray helper is running");
    const response = await messenger.runtime.sendNativeMessage(nativeHost, {
      type: "ensure-running"
    });
    console.warn("[BIRDTRAY-WAYLAND] tray helper startup result", response);
  } catch (error) {
    console.warn("Birdtray helper startup request failed:", error);
  }
})().catch(console.error)
