//
// curve.cpp
//

#include "curve.h"

#include <algorithm>

namespace
{
    // Shared by SCurve and SGradient: finds the two keys around t and how
    // far t is between them. Keys must be sorted. Returns false (and the
    // index of the key to hold) when t is outside the keys or there is only
    // one key.
    template <typename TKey>
    bool findSegment(const std::vector<TKey>& keys, float t, size_t& outA, float& outBlend)
    {
        if (t <= keys.front().time) { outA = 0;               return false; }
        if (t >= keys.back().time)  { outA = keys.size() - 1; return false; }

        // Linear scan — curves have a handful of keys.
        size_t b = 1;
        while (keys[b].time < t) ++b;
        outA = b - 1;

        const float span = keys[b].time - keys[outA].time;
        outBlend = span > 0.0f ? (t - keys[outA].time) / span : 0.0f;   // two keys at the same time — take the first
        return true;
    }

    template <typename TKey>
    void sortByTime(std::vector<TKey>& keys)
    {
        // Stable — two keys at the same time keep their order, which lets a
        // curve jump instantly at that time.
        std::stable_sort(keys.begin(), keys.end(),
                         [](const TKey& a, const TKey& b) { return a.time < b.time; });
    }

    template <typename TKey>
    int insertSorted(std::vector<TKey>& keys, const TKey& key)
    {
        const auto it = std::upper_bound(keys.begin(), keys.end(), key,
                                         [](const TKey& a, const TKey& b) { return a.time < b.time; });
        // Two statements on purpose: in `keys.insert(...) - keys.begin()` the
        // compiler may read begin() before insert() reallocates.
        const auto inserted = keys.insert(it, key);
        return static_cast<int>(inserted - keys.begin());
    }
}

// ---- SCurve -----------------------------------------------------------------

float SCurve::evaluate(float t) const
{
    if (keys.empty()) return 0.0f;

    size_t a     = 0;
    float  blend = 0.0f;
    if (!findSegment(keys, t, a, blend))
        return keys[a].value;

    return keys[a].value + (keys[a + 1].value - keys[a].value) * blend;
}

int SCurve::addKey(float time, float value)
{
    return insertSorted(keys, SCurveKey{ std::clamp(time, 0.0f, 1.0f), value });
}

void SCurve::removeKey(int index)
{
    if (index >= 0 && index < static_cast<int>(keys.size()))
        keys.erase(keys.begin() + index);
}

void SCurve::sortKeys()
{
    sortByTime(keys);
}

void SCurve::getValueRange(float& outMin, float& outMax) const
{
    if (keys.empty()) { outMin = 0.0f; outMax = 1.0f; return; }
    outMin = outMax = keys.front().value;
    for (const SCurveKey& k : keys)
    {
        outMin = std::min(outMin, k.value);
        outMax = std::max(outMax, k.value);
    }
}

// ---- SGradient --------------------------------------------------------------

SVector4 SGradient::evaluate(float t) const
{
    if (keys.empty()) return SVector4(1.0f);

    size_t a     = 0;
    float  blend = 0.0f;
    if (!findSegment(keys, t, a, blend))
        return keys[a].color;

    // Interpolated in the authored (sRGB) space — matches what the editor
    // preview bar shows. Mixing in linear space would give slightly
    // brighter midpoints.
    return keys[a].color + (keys[a + 1].color - keys[a].color) * blend;
}

int SGradient::addKey(float time, const SVector4& color)
{
    return insertSorted(keys, SGradientKey{ std::clamp(time, 0.0f, 1.0f), color });
}

void SGradient::removeKey(int index)
{
    if (index >= 0 && index < static_cast<int>(keys.size()))
        keys.erase(keys.begin() + index);
}

void SGradient::sortKeys()
{
    sortByTime(keys);
}
