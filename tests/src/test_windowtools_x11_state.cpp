#include <gtest/gtest.h>

#include "windowtools_x11_state.h"

TEST(WindowToolsX11State, ProcessOnlyModeUsesHiddenCounter)
{
    EXPECT_TRUE(WindowToolsX11State::isHidden(2, true, false, false));
    EXPECT_FALSE(WindowToolsX11State::isHidden(0, true, false, false));
}

TEST(WindowToolsX11State, MissingX11DisplayAfterProcessExitIsNotHidden)
{
    EXPECT_FALSE(WindowToolsX11State::isHidden(2, false, false, false));
    EXPECT_FALSE(WindowToolsX11State::isHidden(0, false, false, false));
}

TEST(WindowToolsX11State, X11ModeRequiresWindowToBeInactive)
{
    EXPECT_TRUE(WindowToolsX11State::isHidden(2, false, true, false));
    EXPECT_FALSE(WindowToolsX11State::isHidden(2, false, true, true));
}
