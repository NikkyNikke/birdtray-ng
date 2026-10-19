#ifndef BIRDTRAY_QUIT_AUTHORIZATION_H
#define BIRDTRAY_QUIT_AUTHORIZATION_H

inline bool consumeQuitCloseAuthorization(bool &pending) noexcept
{
    if (!pending)
        return false;
    pending = false;
    return true;
}

inline void clearQuitCloseAuthorizationIfWindowGone(bool &pending,
                                                      bool windowExists) noexcept
{
    if (!windowExists)
        pending = false;
}

#endif // BIRDTRAY_QUIT_AUTHORIZATION_H
