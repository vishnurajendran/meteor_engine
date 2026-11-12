#include <GL/glew.h>
#include "lighting_stage.h"
#include <glm/gtc/type_ptr.hpp>
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/graphics/core/render-pipeline/buffer_registry.h"
#include "core/graphics/core/render-pipeline/buffers/buffer_names.h"
#include "core/graphics/core/render-pipeline/buffers/depth/depthbuffer.h"
#include "core/graphics/core/render-pipeline/buffers/frame/frame_buffer.h"
#include "core/graphics/core/render-pipeline/buffers/shadow/shadow_buffer.h"
#include "core/graphics/core/render-pipeline/buffers/texture/texture_buffer.h"
#include "core/graphics/core/render-pipeline/interfaces/render_pipeline_interface.h"
#include "core/graphics/core/render-pipeline/stages/lighting/lighting_system_manager.h"
#include "core/graphics/core/render-pipeline/stages/lighting/shadow_inputs.h"
#include "core/graphics/core/render-pipeline/render_pipeline_manager_interface.h"
#include "core/graphics/core/render-pipeline/view_data.h"
#include "core/graphics/core/render-pipeline/fullscreen_quad.h"
#include "core/graphics/core/shader/shader.h"
#include "core/graphics/core/shader/shaderasset.h"
#include "core/utils/frustum.h"
#include "core/utils/logger.h"

static constexpr const char* LIGHTING_SHADER_PATH =
    "meteor_assets/engine_assets/shaders/internal/lighting_pass.mesl";

// Units 0..8 are the shadow maps (MShadowInputs).
static constexpr int TEX_UNIT_DEPTH        = 9;   // BUFFER_DEPTH
static constexpr int TEX_UNIT_NORMAL       = 10;  // BUFFER_NORMAL
static constexpr int TEX_UNIT_SURFACE      = 11;  // BUFFER_SURFACE
static constexpr int TEX_UNIT_COUNT        = 12;

void MLightingStage::init(IRenderPipeline* const pipeline)
{
    lightsBuffer = pipeline->getBufferRegistry()
                            .createBuffer<SFrameBuffer>(MBufferNames::BUFFER_LIGHTS);
    if (lightsBuffer) lightsBuffer->setColorFormat(0x881A); // GL_RGBA16F

    auto handle = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>()->getAsset<MShaderAsset>(LIGHTING_SHADER_PATH);
    if (handle)
        lightingShaderPath = LIGHTING_SHADER_PATH;
    else
        MERROR("MLightingStage::init -- could not find: " + SString(LIGHTING_SHADER_PATH));

    quad.create();
}

void MLightingStage::cleanup(IRenderPipeline* const pipeline)
{
    quad.destroy();
    lightsBuffer       = nullptr;
    lightingShaderPath = "";
}

void MLightingStage::preRender(IRenderPipeline* const pipeline) {}

void MLightingStage::render(IRenderPipeline* const pipeline)
{
    while (glGetError() != GL_NO_ERROR) {}

    if (!lightsBuffer || lightsBuffer->getFBOHandle() == 0) return;

    const SVector2 res = pipeline->getRenderResolution();
    const int w = static_cast<int>(res.x);
    const int h = static_cast<int>(res.y);

    // ---- Clear: alpha 0 = unlit, so the composite shows plain albedo -------
    lightsBuffer->bindAsActive();
    glViewport(0, 0, w, h);
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT);

    // ---- Inputs -------------------------------------------------------------
    auto& registry = pipeline->getBufferRegistry();
    auto* depth    = registry.getBuffer<SDepthBuffer>  (MBufferNames::BUFFER_DEPTH);
    auto* normal   = registry.getBuffer<STextureBuffer>(MBufferNames::BUFFER_NORMAL);
    auto* surface  = registry.getBuffer<STextureBuffer>(MBufferNames::BUFFER_SURFACE);

    const bool hasPrepass = (pipeline->getCompositeFlags() & ECF_Prepass) &&
                            depth   && depth->getDepthTextureHandle()   != 0 &&
                            normal  && normal->getTextureHandle()       != 0 &&
                            surface && surface->getTextureHandle()      != 0;

    auto lightingAsset = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>()
                              ->getAsset<MShaderAsset>(lightingShaderPath.c_str());
    MShader* lightingShader = lightingAsset ? lightingAsset->getShader() : nullptr;

    auto* renderer = MEngineSubsystemRegistry::getSubsystem<IRenderPipelineManagerSubsystem>();

    if (!hasPrepass || !lightingShader || !renderer || !quad.isValid())
    {
        // Warn once per stage instance, not every frame.
        if (!warnedMissingInputs)
        {
            if (!hasPrepass)
                MWARN("MLightingStage::render -- prepass buffers missing; add MPrepassStage before MLightingStage");
            else if (!lightingShader)
                MWARN("MLightingStage::render -- lightingShader is null, rendering without lighting");
            warnedMissingInputs = true;
        }
        lightsBuffer->unbind();
        return;
    }

    auto* mgr = &renderer->getLightSystem();
    const SViewData viewData = resolveViewData(pipeline);

    // ---- Point / spot lights for the whole view, uploaded once ------------
    // Runs after MShadowStage, so each light's shadow index is current.
    if (pipeline->getLightOverride().active || !viewData.valid)
    {
        mgr->prepareViewDynamicLights(nullptr, SVector3(0.f));
    }
    else
    {
        SFrustum frustum;
        frustum.extractFromVP(viewData.projection * viewData.view);
        mgr->prepareViewDynamicLights(&frustum, viewData.cameraPosition);
    }

    // ---- Fullscreen state ---------------------------------------------------
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    lightingShader->bind();

    // The quad is already in NDC - identity transform through the base shader.
    {
        SShaderPropertyValue identity;
        identity.setMat4Val(glm::mat4(1.f));
        lightingShader->setPropertyValue("model",      identity);
        lightingShader->setPropertyValue("view",       identity);
        lightingShader->setPropertyValue("projection", identity);
    }

    GLint prog = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prog);

    // ---- Prepass buffers + camera -----------------------------------------
    {
        glActiveTexture(GL_TEXTURE0 + TEX_UNIT_DEPTH);
        glBindTexture(GL_TEXTURE_2D, depth->getDepthTextureHandle());
        glActiveTexture(GL_TEXTURE0 + TEX_UNIT_NORMAL);
        glBindTexture(GL_TEXTURE_2D, normal->getTextureHandle());
        glActiveTexture(GL_TEXTURE0 + TEX_UNIT_SURFACE);
        glBindTexture(GL_TEXTURE_2D, surface->getTextureHandle());

        GLint l;
        l = glGetUniformLocation(prog, "_depthTex");   if (l >= 0) glUniform1i(l, TEX_UNIT_DEPTH);
        l = glGetUniformLocation(prog, "_normalTex");  if (l >= 0) glUniform1i(l, TEX_UNIT_NORMAL);
        l = glGetUniformLocation(prog, "_surfaceTex"); if (l >= 0) glUniform1i(l, TEX_UNIT_SURFACE);

        l = glGetUniformLocation(prog, "_invView");
        if (l >= 0) glUniformMatrix4fv(l, 1, GL_FALSE, glm::value_ptr(viewData.inverseView));
        l = glGetUniformLocation(prog, "_invProjection");
        if (l >= 0) glUniformMatrix4fv(l, 1, GL_FALSE, glm::value_ptr(viewData.inverseProjection));
        l = glGetUniformLocation(prog, "_cameraPos");
        if (l >= 0) glUniform3fv(l, 1, glm::value_ptr(viewData.cameraPosition));
    }

    // ---- Shadow maps (units 0..8, shared with MTransparentStage) -----------
    MShadowInputs::bind(pipeline, static_cast<unsigned int>(prog));

    // ---- One fullscreen draw ------------------------------------------------
    quad.draw();
    glUseProgram(0);

    // ---- Restore ------------------------------------------------------------
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);

    for (int i = 0; i < TEX_UNIT_COUNT; ++i)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D,       0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }
    glActiveTexture(GL_TEXTURE0);

    lightsBuffer->unbind();
    pipeline->addCompositeFlag(ECF_Lights);
}

void MLightingStage::postRender(IRenderPipeline* const pipeline) {}
