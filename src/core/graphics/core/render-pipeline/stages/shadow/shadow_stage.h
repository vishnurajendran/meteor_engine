#pragma once
#ifndef SHADOW_STAGE_H
#define SHADOW_STAGE_H

#include <cstdint>
#include <unordered_set>
#include <utility>
#include <vector>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include "core/utils/sstring.h"
#include "core/graphics/core/render-pipeline/buffers/shadow/shadow_buffer.h"
#include "core/graphics/core/render-pipeline/stages/render_stage.h"
#include "core/graphics/core/render-pipeline/stages/lighting/light_type.h"

class SShadowBuffer;
struct SFrustum;
class MShader;
class MLightEntity;
class MDynamicLight;

class MShadowStage : public MRenderStage
{
    DEFINE_OBJECT_SUBCLASS(MShadowStage)
public:
    int  getSortingOrder() override { return ERenderStageOrder::RS_Shadow; }

    void init      (IRenderPipeline* const pipeline) override;
    void cleanup   (IRenderPipeline* const pipeline) override;
    void preRender (IRenderPipeline* const pipeline) override;
    void render    (IRenderPipeline* const pipeline) override;
    void postRender(IRenderPipeline* const pipeline) override;

    // ---- Rolling spot / point shadow updates ---------------------------------
    // A light keeps its shadow map (slot) while it stays selected, so the map
    // can be reused: only *_UPDATES_PER_FRAME maps are re-rendered per frame -
    // new owners first, then lights that moved / turned / changed range, the
    // top-ranked light, then the oldest map. A light waiting for its first
    // render casts no shadow. Moving objects' shadows in cached maps lag by at
    // most (slots / updates) frames.
    static constexpr int SPOT_UPDATES_PER_FRAME  = 2;
    static constexpr int POINT_UPDATES_PER_FRAME = 1;   // 6 scene passes each

    struct SShadowSlot
    {
        const MDynamicLight* owner = nullptr;   // compared only, never dereferenced unless selected
        SString   ownerGuid;                    // guards against a new light at a freed address
        bool      valid = false;                // holds a rendered map for owner
        glm::vec3 position  = glm::vec3(0.f);   // what the map was rendered with
        glm::vec3 direction = glm::vec3(0.f);
        float     range = 0.f;
        float     angle = 0.f;
        uint64_t  lastRendered = 0;
    };

private:
    void renderDirectionalShadow(IRenderPipeline* const pipeline, MLightEntity* dirLight);
    void renderSpotShadows      (IRenderPipeline* const pipeline);
    void renderPointShadows     (IRenderPipeline* const pipeline);

    // For mesl-based shaders (directional, spot).
    // cullFrustum — the light's own frustum; items whose bounds are entirely
    // outside it are skipped. nullptr draws every caster.
    void drawItems(IRenderPipeline* const pipeline, MShader* shader,
                   unsigned int* rawProg, bool shadowCastersOnly,
                   const SFrustum* cullFrustum = nullptr);

    // For the inline point shadow program - sets model via raw GL uniform.
    void drawItemsRaw(IRenderPipeline* const pipeline,
                      unsigned int prog, bool shadowCastersOnly,
                      const SFrustum* cullFrustum = nullptr);

    // Which lights of `type` get one of `slots` shadow maps this frame, best
    // first. Candidates: enabled, casting shadows, range sphere in the camera
    // frustum. Ranked by shadowPriority, then by distance from the camera to
    // the light's range sphere (lights last frame's set get a head start, so
    // two lights at similar distance don't swap shadows every frame).
    // Clears the shadow index of every light of `type`; the caller sets it
    // for the ones it renders.
    struct SShadowSelection
    {
        std::unordered_set<const MDynamicLight*> lastSelected;
        int lastSkipped = 0;   // for logging only when it changes
    };
    std::vector<MDynamicLight*> selectShadowCasters(IRenderPipeline* pipeline, ELightType type,
                                                    int slots, SShadowSelection& state);

    SShadowSelection spotSelection;
    SShadowSelection pointSelection;

    // Assigns slots to `selected` (keeping existing ones) and lists which
    // (light, slot) pairs to render this frame, within `budget`.
    void planSlotUpdates(const std::vector<MDynamicLight*>& selected,
                         SShadowSlot* slots, int slotCount, int budget,
                         std::vector<std::pair<MDynamicLight*, int>>& outRender);

    SShadowSlot spotSlots [SShadowBuffer::MAX_SPOT_SHADOWS];
    SShadowSlot pointSlots[SShadowBuffer::MAX_POINT_SHADOWS];
    uint64_t    frameIndex = 1;   // slots start at 0, so a fresh map always looks oldest

    SShadowBuffer* shadowBuffer       = nullptr;
    MShader*       shadowShader       = nullptr;
    unsigned int   pointShadowProgram = 0; // inline GLSL, writes linear depth
};

#endif