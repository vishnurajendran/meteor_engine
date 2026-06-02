//
// Created by ssj5v on 11-04-2026.
//

#pragma once
#ifndef RENDER_PIPELINE_INTERFACE_H
#define RENDER_PIPELINE_INTERFACE_H

#include <cstdint>
#include <vector>

#include "core/graphics/core/render-pipeline/render_item.h"
#include "render_stage_interface.h"

class MBufferRegistery;

enum ECompositeFlags : uint32_t
{
    ECF_None        = 0,
    ECF_Depth       = 1 << 0,
    ECF_Opaque      = 1 << 1,
    ECF_Lights      = 1 << 2,
    ECF_Shadow      = 1 << 3,
    ECF_Transparent = 1 << 4,
};

class IRenderPipeline
{
public:
    virtual ~IRenderPipeline() = default;

    virtual void init()=0;
    virtual void cleanup()=0;

    template<typename T>
    bool addStage()
    {
        static_assert(std::is_base_of_v<IRenderStage, T>,
                      "T must derive from IRenderStage");
        IRenderStage* stage = new T();
        stage->init(this);
        renderStages.push_back(stage);
        std::sort(renderStages.begin(), renderStages.end(),
            [](IRenderStage* a, IRenderStage* b) { return a->getSortingOrder() < b->getSortingOrder(); });
        return true;
    }

    // Frame lifecycle
    virtual void preRender()  = 0;
    virtual void render()     = 0;
    virtual void postRender() = 0;

    // Render target
    virtual SVector2       getRenderResolution()                  = 0;
    virtual void           setRenderBuffer(SRenderBuffer* buffer) = 0;
    virtual SRenderBuffer* getRenderBuffer()                      = 0;

    // Buffer registry
    virtual MBufferRegistery& getBufferRegistry() = 0;

    // Render items
    virtual const std::vector<SRenderItem>& getRenderItems() const = 0;

    // Composite flags
    virtual uint32_t getCompositeFlags()                  const = 0;
    virtual void     addCompositeFlag(ECompositeFlags flag)     = 0;
    virtual void     removeCompositeFlag(ECompositeFlags flag)  = 0;
    virtual void     clearCompositeFlags()                      = 0;

    // ---- Camera override ----------------------------------------------------
    struct SCameraOverride
    {
        bool      active = false;
        glm::mat4 view   = glm::mat4(1.f);
        glm::mat4 proj   = glm::mat4(1.f);
    };

    void setCameraOverride(const glm::mat4& view, const glm::mat4& proj)
    {
        cameraOverride.active = true;
        cameraOverride.view   = view;
        cameraOverride.proj   = proj;
    }
    void clearCameraOverride() { cameraOverride.active = false; }
    const SCameraOverride& getCameraOverride() const { return cameraOverride; }

    // ---- Light override -----------------------------------------------------
    // When active, the opaque stage writes these values to the ambient and
    // directional UBOs instead of calling MLightSystemManager::prepareLights(),
    // and skips dynamic light preparation entirely.
    struct SLightOverride
    {
        bool active = false;

        glm::vec3 ambientColor     = glm::vec3(1.0f);
        float     ambientIntensity = 0.2f;

        glm::vec3 directionalDirection = glm::normalize(glm::vec3(-1.0f, 2.0f, 2.0f));
        glm::vec3 directionalColor     = glm::vec3(1.0f);
        float     directionalIntensity = 1.0f;
    };

    void setLightOverride(const glm::vec3& ambCol,  float ambInt,
                          const glm::vec3& dirDir,
                          const glm::vec3& dirCol,   float dirInt)
    {
        lightOverride.active                = true;
        lightOverride.ambientColor          = ambCol;
        lightOverride.ambientIntensity      = ambInt;
        lightOverride.directionalDirection  = dirDir;
        lightOverride.directionalColor      = dirCol;
        lightOverride.directionalIntensity  = dirInt;
    }
    void clearLightOverride() { lightOverride.active = false; }
    const SLightOverride& getLightOverride() const { return lightOverride; }

protected:
    std::vector<IRenderStage*> renderStages;
    SCameraOverride            cameraOverride;
    SLightOverride             lightOverride;
};

#endif // RENDER_PIPELINE_INTERFACE_H