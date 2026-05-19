//
// Created by ssj5v on 17-05-2026.
//

#ifndef AUDIOSOURCE_INTERFACE_H
#define AUDIOSOURCE_INTERFACE_H
#include "core/engine/audio/asset/audioclip_asset.h"
#include "core/engine/assetmanagement/asset/asset_handle.h"
#include "core/utils/glmhelper.h"

class IAudioSource
{
public:
    virtual ~IAudioSource() = default;
    virtual void init() = 0;
    virtual void cleanup() = 0;

    virtual void tick(const float& deltaTime)=0;
    virtual void setClip(TAssetHandle<MAudioClipAsset> clip)=0;

    // Playback
    virtual void play() = 0;
    virtual void stop() = 0;

    virtual void setLooping(const bool& looping)=0;
    virtual void setVolume(const float& volume)=0;
    virtual void setPitch(const float& pitch) =0;

    // Spatialization
    virtual void setSpatializationEnabled(const bool& enabled)=0;
    virtual void setRollOff(const float& rollOff)=0;
    virtual void setMinDist(const float& minDist)=0;
    virtual void setMaxDist(const float& maxDist)=0;
    virtual void setDopplerStrength(const float& strength)=0;
    virtual void setPosition(const SVector3& position)=0;
    virtual void setDirection(const SVector3& direction) = 0;
    virtual void setVelocity(const SVector3& velocity) = 0;
};

#endif //AUDIOSOURCE_INTERFACE_H
