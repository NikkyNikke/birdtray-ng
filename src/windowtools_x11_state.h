#ifndef WINDOWTOOLS_X11_STATE_H
#define WINDOWTOOLS_X11_STATE_H

namespace WindowToolsX11State {

inline bool isHidden(int hiddenStateCounter, bool processOnly,
                     bool x11DisplayAvailable, bool x11WindowIsActive)
{
    if (processOnly)
        return hiddenStateCounter == 2;
    if (!x11DisplayAvailable)
        return false;
    return hiddenStateCounter == 2 && !x11WindowIsActive;
}

}

#endif // WINDOWTOOLS_X11_STATE_H
