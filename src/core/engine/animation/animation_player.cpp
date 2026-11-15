#include "animation_player.h"

#include <algorithm>
#include <cmath>

void SAnimationPlayer::dropEntry(int index)
{
    for (int i = index; i + 1 < entryCount; ++i)
        entries[i] = entries[i + 1];
    --entryCount;
}

void SAnimationPlayer::normalizeWeights()
{
    float total = 0.0f;
    for (int i = 0; i < entryCount; ++i) total += entries[i].weight;
    if (total <= 0.0f)
    {
        // Nothing left with weight: the newest entry takes over fully.
        if (entryCount > 0)
        {
            for (int i = 0; i < entryCount; ++i) entries[i].weight = 0.0f;
            entries[entryCount - 1].weight = 1.0f;
        }
        return;
    }
    for (int i = 0; i < entryCount; ++i) entries[i].weight /= total;
}

void SAnimationPlayer::afterRemoval(bool targetRemoved)
{
    if (entryCount == 0)
    {
        clear();
        return;
    }
    if (targetRemoved)
        hasFadeTarget = false;   // the clip being faded to is gone: the rest fade out

    if (fadeProgress >= 1.0f)
    {
        normalizeWeights();
        return;
    }

    // Mid-fade: the remaining fading-out entries share what they had.
    const int fadingOut = hasFadeTarget ? entryCount - 1 : entryCount;
    float total = 0.0f;
    for (int i = 0; i < fadingOut; ++i) total += entries[i].baseWeight;
    for (int i = 0; i < fadingOut; ++i)
        entries[i].baseWeight = total > 0.0f ? entries[i].baseWeight / total : 1.0f / static_cast<float>(fadingOut);
    applyFadeWeights();
}

void SAnimationPlayer::startFade(float blendTime, bool withTarget)
{
    // Every entry that fades out keeps its current share of the weight.
    const int fadingOut = withTarget ? entryCount - 1 : entryCount;
    float total = 0.0f;
    for (int i = 0; i < fadingOut; ++i) total += entries[i].weight;
    for (int i = 0; i < fadingOut; ++i)
        entries[i].baseWeight = total > 0.0f ? entries[i].weight / total : 1.0f / static_cast<float>(fadingOut);

    hasFadeTarget = withTarget;
    if (blendTime <= 0.0f)
    {
        fadeProgress = 1.0f;
        fadeRate     = 0.0f;
    }
    else
    {
        fadeProgress = 0.0f;
        fadeRate     = 1.0f / blendTime;
    }
    applyFadeWeights();
}

void SAnimationPlayer::applyFadeWeights()
{
    const float p = std::clamp(fadeProgress, 0.0f, 1.0f);
    const int fadingOut = hasFadeTarget ? entryCount - 1 : entryCount;
    for (int i = 0; i < fadingOut; ++i)
        entries[i].weight = entries[i].baseWeight * (1.0f - p);
    if (hasFadeTarget && entryCount > 0)
        entries[entryCount - 1].weight = p;

    if (p >= 1.0f)
    {
        // Fade done: drop everything that reached 0.
        for (int i = entryCount - 1; i >= 0; --i)
            if (entries[i].weight <= 0.0f)
                dropEntry(i);
        if (entryCount == 0)
            hasFadeTarget = false;
    }
}

bool SAnimationPlayer::play(const SString& clipName, const MAnimClip* clip, const SClipBinding* binding,
                            float duration, float blendTime, bool loop)
{
    if (entryCount > 0 && hasFadeTarget && entries[entryCount - 1].clipName == clipName)
        return false;   // already playing (or fading in)
    if (entryCount == 1 && !hasFadeTarget && fadeProgress >= 1.0f && entries[0].clipName == clipName)
        return false;

    if (entryCount == MAX_ENTRIES)
    {
        dropEntry(0);   // a fourth crossfade mid-blend drops the oldest
        normalizeWeights();
    }

    SAnimationPlayerEntry& entry = entries[entryCount++];
    entry = SAnimationPlayerEntry{};
    entry.clipName = clipName;
    entry.clip     = clip;
    entry.binding  = binding;
    entry.duration = duration;
    entry.loop     = loop;

    if (entryCount == 1)
        blendTime = 0.0f;   // nothing to blend from (bind pose is not faded from)
    startFade(blendTime, true);
    return true;
}

void SAnimationPlayer::stop(float blendTime)
{
    if (entryCount == 0)
        return;
    startFade(blendTime, false);
}

void SAnimationPlayer::advance(float deltaTime, std::vector<SString>& finishedClips)
{
    if (entryCount == 0)
        return;

    const float step = deltaTime * speed;
    for (int i = 0; i < entryCount; ++i)
    {
        SAnimationPlayerEntry& e = entries[i];
        if (e.duration <= 0.0f)
        {
            e.time = 0.0f;
            if (!e.loop && !e.finished)
            {
                e.finished = true;
                finishedClips.push_back(e.clipName);
            }
            continue;
        }

        e.time += step;
        if (e.loop)
        {
            e.time = std::fmod(e.time, e.duration);
            if (e.time < 0.0f) e.time += e.duration;
        }
        else if (e.time >= e.duration)
        {
            e.time = e.duration;
            if (!e.finished)
            {
                e.finished = true;
                finishedClips.push_back(e.clipName);
            }
        }
        else if (e.time < 0.0f)
        {
            e.time = 0.0f;   // playing backwards stops at the start
        }
    }

    if (fadeProgress < 1.0f)
    {
        // Fades run on real time, not clip speed.
        fadeProgress = std::min(1.0f, fadeProgress + std::abs(deltaTime) * fadeRate);
        applyFadeWeights();
    }
}

int SAnimationPlayer::dominantEntry() const
{
    int best = -1;
    float bestWeight = -1.0f;
    for (int i = 0; i < entryCount; ++i)
    {
        // >= : on a tie the newer entry wins
        if (entries[i].weight >= bestWeight)
        {
            bestWeight = entries[i].weight;
            best = i;
        }
    }
    return best;
}
