#include <gtest/gtest.h>

#include "hide_retry_policy.h"

TEST(HideRetryPolicy, ConfirmedImmediate)
{
    EXPECT_EQ(HideRetryPolicy::Step::Hidden, HideRetryPolicy::evaluate(1, true));
}

TEST(HideRetryPolicy, ConfirmedLateStillWins)
{
    EXPECT_EQ(HideRetryPolicy::Step::Hidden,
              HideRetryPolicy::evaluate(HideRetryPolicy::maxAttempts, true));
}

TEST(HideRetryPolicy, RetriesInsideBudget)
{
    for (int attempt = 1; attempt < HideRetryPolicy::maxAttempts; ++attempt)
    {
        EXPECT_EQ(HideRetryPolicy::Step::Retry, HideRetryPolicy::evaluate(attempt, false));
    }
}

TEST(HideRetryPolicy, ExhaustsAtLimit)
{
    EXPECT_EQ(HideRetryPolicy::Step::Exhausted,
              HideRetryPolicy::evaluate(HideRetryPolicy::maxAttempts, false));
}

TEST(HideRetryPolicy, BudgetIsBounded)
{
    EXPECT_EQ(HideRetryPolicy::maxAttempts * HideRetryPolicy::attemptSpacingMs,
              HideRetryPolicy::totalBudgetMs());
    EXPECT_EQ(1250, HideRetryPolicy::totalBudgetMs());
}
