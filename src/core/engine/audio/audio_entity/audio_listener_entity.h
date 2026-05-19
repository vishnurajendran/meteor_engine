//
// Created by ssj5v on 18-05-2026.
//

#ifndef AUDIO_LISTENER_ENTITY_H
#define AUDIO_LISTENER_ENTITY_H
#include "core/engine/audio/interfaces/audiolistener_interface.h"
#include "core/engine/audio/interfaces/engine_interface.h"
#include "core/engine/entities/spatial/spatial.h"

class MAudioListener : public MSpatialEntity {
    DEFINE_SPATIAL_CLASS(MAudioListener)
public:
    MAudioListener() = default;
    ~MAudioListener() override = default;

    void onCreate() override;
    void onExit() override;
    void onUpdate(float deltaTime) override;

    void onDrawGizmo(SVector2 renderResolution) override;

private:
    IAudioListener* listener = nullptr;
    IAudioEngineSubsystem* engineSubsystem = nullptr;

    SVector3 lastPosition = {0,0,0};
    bool initialized = false;
};



#endif //AUDIO_LISTENER_ENTITY_H
