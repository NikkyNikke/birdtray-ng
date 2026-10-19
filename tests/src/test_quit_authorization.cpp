#include <gtest/gtest.h>

#include "quit_authorization.h"

TEST(QuitCloseAuthorization, ExplicitCloseIsConsumedOnce)
{
    bool pending = true;

    EXPECT_TRUE(consumeQuitCloseAuthorization(pending));
    EXPECT_FALSE(pending);
    EXPECT_FALSE(consumeQuitCloseAuthorization(pending));
}

TEST(QuitCloseAuthorization, DoesNotAuthorizeWithoutExplicitQuit)
{
    bool pending = false;

    EXPECT_FALSE(consumeQuitCloseAuthorization(pending));
    EXPECT_FALSE(pending);
}

TEST(QuitCloseAuthorization, ClearsPendingQuitAfterThunderbirdExits)
{
    bool pending = true;

    clearQuitCloseAuthorizationIfWindowGone(pending, false);

    EXPECT_FALSE(pending);
}

TEST(QuitCloseAuthorization, KeepsPendingQuitWhileThunderbirdIsOpen)
{
    bool pending = true;

    clearQuitCloseAuthorizationIfWindowGone(pending, true);

    EXPECT_TRUE(pending);
}
