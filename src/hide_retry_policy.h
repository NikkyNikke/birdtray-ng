#ifndef BIRDTRAY_HIDE_RETRY_POLICY_H
#define BIRDTRAY_HIDE_RETRY_POLICY_H

/**
 * Decision logic for answering add-on close requests.
 *
 * A close-request is answered with "hidden" only when the hide script
 * confirms that every matching window ended up minimized and excluded from
 * the taskbar, pager and switcher. The very first attempt may still miss
 * (for example when the window list is not fully populated yet right after
 * an application start), so unconfirmed attempts retry for a short and
 * strictly bounded time window. If the budget runs out without a
 * confirmation, the caller answers with the unchanged minimize fallback.
 * An explicit user gesture (a tray icon click) cancels a pending reply and
 * also answers with the fallback. This keeps the reply protocol identical
 * to the pre-retry behaviour for all non-race situations.
 */
namespace HideRetryPolicy
{
enum class Step
{
    Hidden,    ///< Confirmed: answer with "hidden".
    Retry,     ///< Unconfirmed, but attempts remain: schedule the next one.
    Exhausted, ///< Unconfirmed and the budget is used up: fall back.
};

/**
 * How many attempts happen in total, including the immediate first one.
 */
constexpr int maxAttempts = 5;

/**
 * Spacing between the follow-up attempts; attempt 1 happens immediately.
 */
constexpr int attemptSpacingMs = 250;

/**
 * Upper bound for how long a close request may wait for its final answer
 * before it falls back.
 */
constexpr int totalBudgetMs()
{
    return maxAttempts * attemptSpacingMs;
}

/**
 * Evaluate the next step after an attempt.
 *
 * @param attempt   The 1-based number of the finished attempt.
 * @param confirmed Whether the hide attempt was confirmed.
 */
constexpr Step evaluate(const int attempt, const bool confirmed)
{
    if (confirmed)
    {
        return Step::Hidden;
    }
    return attempt < maxAttempts ? Step::Retry : Step::Exhausted;
}
}

#endif /* BIRDTRAY_HIDE_RETRY_POLICY_H */
