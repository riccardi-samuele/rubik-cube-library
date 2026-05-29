#pragma once

#include <string>

namespace rubik::detail {

enum class OptimalSchedulerDecision {
    Root,
    DeepSplit,
};

struct AdaptiveDeepSplitInputs {
    int initialLowerBound = 0;
    int maxDepth = 0;
    unsigned int threads = 0;
    int strongMinCount = 0;
    bool firstMoveDiffers = false;
};

struct AdaptiveDeepSplitDecision {
    OptimalSchedulerDecision scheduler = OptimalSchedulerDecision::Root;
    std::string reason = "default_root";
    int initialLowerBound = 0;
    int maxDepth = 0;
    unsigned int threads = 0;
    int strongMinCount = 0;
    bool firstMoveDiffers = false;
};

enum class AdaptiveRootOrderingDecision {
    Default,
    ReverseTie,
    HighBoundFirst,
};

inline AdaptiveDeepSplitDecision chooseAdaptiveDeepSplit(const AdaptiveDeepSplitInputs& inputs)
{
    AdaptiveDeepSplitDecision decision{
        .scheduler = OptimalSchedulerDecision::Root,
        .reason = "conservative_root",
        .initialLowerBound = inputs.initialLowerBound,
        .maxDepth = inputs.maxDepth,
        .threads = inputs.threads,
        .strongMinCount = inputs.strongMinCount,
        .firstMoveDiffers = inputs.firstMoveDiffers,
    };

    const int remainingDepth = inputs.maxDepth - inputs.initialLowerBound;
    if (inputs.threads < 4) {
        decision.reason = "threads_lt_4";
        return decision;
    }
    if (remainingDepth < 5) {
        decision.reason = "remaining_depth_lt_5";
        return decision;
    }
    if (inputs.initialLowerBound == 8 &&
        !inputs.firstMoveDiffers &&
        inputs.strongMinCount >= 6 &&
        inputs.strongMinCount <= 8) {
        decision.scheduler = OptimalSchedulerDecision::DeepSplit;
        decision.reason = "lb8_stable_mid_strong_min";
        return decision;
    }
    if (inputs.initialLowerBound == 9 && inputs.strongMinCount <= 1) {
        decision.scheduler = OptimalSchedulerDecision::DeepSplit;
        decision.reason = "lb9_low_strong_min";
        return decision;
    }
    if (inputs.initialLowerBound == 9 &&
        inputs.strongMinCount >= 4 &&
        inputs.strongMinCount <= 7) {
        decision.scheduler = OptimalSchedulerDecision::DeepSplit;
        decision.reason = "lb9_mid_strong_min";
        return decision;
    }
    if (inputs.maxDepth == 14 &&
        inputs.initialLowerBound >= 8 &&
        inputs.initialLowerBound <= 9 &&
        inputs.strongMinCount >= 2) {
        decision.scheduler = OptimalSchedulerDecision::DeepSplit;
        decision.reason = "depth14_conservative_root";
        return decision;
    }

    return decision;
}

inline AdaptiveRootOrderingDecision chooseAdaptiveRootOrdering(const AdaptiveDeepSplitInputs& inputs)
{
    if (inputs.threads < 4) {
        return AdaptiveRootOrderingDecision::Default;
    }

    const int remainingDepth = inputs.maxDepth - inputs.initialLowerBound;
    if (remainingDepth < 5) {
        return AdaptiveRootOrderingDecision::Default;
    }

    if (inputs.initialLowerBound == 8 &&
        !inputs.firstMoveDiffers &&
        inputs.strongMinCount >= 6 &&
        inputs.strongMinCount <= 8) {
        return AdaptiveRootOrderingDecision::HighBoundFirst;
    }

    if (inputs.initialLowerBound == 9 &&
        !inputs.firstMoveDiffers &&
        inputs.strongMinCount >= 4 &&
        inputs.strongMinCount <= 7) {
        return AdaptiveRootOrderingDecision::ReverseTie;
    }

    return AdaptiveRootOrderingDecision::Default;
}

} // namespace rubik::detail
