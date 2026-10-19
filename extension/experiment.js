// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// Based on the ex_windows experiment from "Minimize on Close" by
// rsjtdrjgfuzkfg (Dirk Steinmetz), Mozilla Public License 2.0. This
// copy only adds labeled diagnostic logging and is used by the
// Birdtray Wayland fork.
const { ExtensionCommon } = ChromeUtils.importESModule(
    "resource://gre/modules/ExtensionCommon.sys.mjs");

this.ex_windows = class extends ExtensionCommon.ExtensionAPI {
  getAPI(context) {
    // Note: this implementation is unsafe if multiple add-ons would lock the
    // same window and unlock in a different order. Fixing this is not
    // reasonable for now, as this is an experiment used by a single add-on
    // only.
    const extension = context.extension;

    let listeners = []; // objects with 'async' callback method for onClosing

    // Called when a locked DOM window would close
    const onClosing =  function(window) {
      for (let listener of listeners) {
        listener.async(extension.windowManager.getWrapper(window).id);
      }
    };

    // Close event handler for locked DOM windows
    const closeEventHandler = function(event) {
      console.warn("[BIRDTRAY-WAYLAND] close event entered", {
        type: event.type,
        cancelable: event.cancelable,
        defaultPreventedBefore: event.defaultPrevented
      });
      event.preventDefault();
      console.warn("[BIRDTRAY-WAYLAND] after preventDefault", {
        cancelable: event.cancelable,
        defaultPreventedAfter: event.defaultPrevented
      });
      onClosing(event.target);
    };

    // Locks a DOM window
    const lockWindow = function(window) {
      // We redirect two things: the close event (used in all sane situations)
      // and the window.close method (used when tabs are in the toolbar). The
      // latter is ugly, but all other methods to "fix" the close toolbar button
      // don't seem to work fast enough to prevent the window from closing.
      console.warn("[BIRDTRAY-WAYLAND] lockWindow", window.location && window.location.href);
      window.addEventListener("close", closeEventHandler, false);
      window.__minimize_on_close__originalClose = window.close;
      window.close = function() {
        console.warn("[BIRDTRAY-WAYLAND] window.close override entered");
        onClosing(this);
      };
    };

    // Unlocks a DOM window
    const unlockWindow = function(window) {
      const document = window.document;
      window.removeEventListener("close", closeEventHandler, false);
      window.close = window.__minimize_on_close__originalClose;
      delete window.__minimize_on_close__originalClose;
    }

    let lockedWindows = []; // DOM windows that have been locked

    context.callOnClose({
      close() {
        for (let window of lockedWindows) {
          unlockWindow(window);
        }
        lockedWindows = [];
      }
    });

    return {
      ex_windows: {
        lockWindow(windowId) {
          const window = extension.windowManager.get(windowId, context).window;
          lockWindow(window);
          lockedWindows.push(window);
        },

        unlockWindow(windowId) {
          const window = extension.windowManager.get(windowId, context).window;
          const index = lockedWindows.indexOf(window);
          if (index >= 0) {
            unlockWindow(window);
            lockedWindows.splice(index, 1);
          }
        },

        onClosing: new ExtensionCommon.EventManager({
          context,
          name: "ex_windows.onClosing",
          register(fire) {
            listeners.push(fire);
            return function() {
              const index = listeners.indexOf(fire);
              if (index >= 0) {
                listeners.splice(index, 1);
              }
            };
          }
        }).api()
      }
    };
  }
};
