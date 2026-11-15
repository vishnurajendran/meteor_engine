#include "anim_clip.h"

#include <algorithm>
#include <cmath>

namespace
{
    // Index and blend factor for `time` on an evenly sampled channel.
    struct SKeyPair
    {
        int   a = 0;
        int   b = 0;
        float t = 0.0f;
    };

    SKeyPair keysAt(float time, float rate, int keyCount)
    {
        SKeyPair k;
        if (keyCount <= 1)
            return k;
        const float f    = std::max(time, 0.0f) * rate;
        const int   last = keyCount - 1;
        k.a = std::min(static_cast<int>(f), last);
        k.b = std::min(k.a + 1, last);
        k.t = k.a == last ? 0.0f : f - static_cast<float>(k.a);
        return k;
    }

    template <typename T>
    T lerpKeys(const std::vector<T>& keys, const SKeyPair& k)
    {
        return keys[k.a] + (keys[k.b] - keys[k.a]) * k.t;
    }
}

STransform MAnimClip::sampleTrack(int trackIndex, float time) const
{
    const STrack& track = tracks[trackIndex];
    STransform out;

    if (!track.positions.empty())
        out.position = track.positions.size() == 1
            ? track.positions[0]
            : lerpKeys(track.positions, keysAt(time, sampleRate, (int)track.positions.size()));

    if (!track.rotations.empty())
    {
        if (track.rotations.size() == 1)
            out.rotation = track.rotations[0];
        else
        {
            const SKeyPair k = keysAt(time, sampleRate, (int)track.rotations.size());
            // Normalized quaternion blend; neighbouring keys are already on
            // the same hemisphere (made so at import).
            out.rotation = glm::normalize(glm::slerp(track.rotations[k.a], track.rotations[k.b], k.t));
        }
    }

    if (!track.scales.empty())
        out.scale = track.scales.size() == 1
            ? track.scales[0]
            : lerpKeys(track.scales, keysAt(time, sampleRate, (int)track.scales.size()));

    return out;
}
