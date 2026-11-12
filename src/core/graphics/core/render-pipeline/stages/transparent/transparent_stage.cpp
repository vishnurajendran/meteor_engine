//
// Transparent stage - RS_Transparent (4500).
//

#include <GL/glew.h>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "transparent_stage.h"

#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/engine/texture/texture.h"
#include "core/engine/texture/textureasset.h"
#include "core/graphics/core/material/material.h"
#include "core/graphics/core/render-pipeline/buffer_registry.h"
#include "core/graphics/core/render-pipeline/buffers/buffer_names.h"
#include "core/graphics/core/render-pipeline/buffers/frame/frame_buffer.h"
#include "core/graphics/core/render-pipeline/interfaces/render_pipeline_interface.h"
#include "core/graphics/core/render-pipeline/render_item.h"
#include "core/graphics/core/render-pipeline/render_pipeline_manager_interface.h"
#include "core/graphics/core/render-pipeline/stages/lighting/lighting_system_manager.h"
#include "core/graphics/core/render-pipeline/stages/lighting/shadow_inputs.h"
#include "core/graphics/core/render-pipeline/view_data.h"
#include "core/graphics/core/shader/shader.h"
#include "core/graphics/core/shader/default_textures.h"
#include "core/utils/logger.h"

// Material textures use units MShadowInputs::UNIT_COUNT .. MAX_TEX_UNIT.
static constexpr int MAX_TEX_UNIT = 31;

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void MTransparentStage::init(IRenderPipeline* const pipeline)
{
    sceneBuffer = pipeline->getBufferRegistry()
                           .getBuffer<SFrameBuffer>(MBufferNames::BUFFER_SCENE);
    if (!sceneBuffer)
        MERROR("MTransparentStage::init - BUFFER_SCENE not found; add MResolveStage before MTransparentStage");
}

void MTransparentStage::cleanup(IRenderPipeline* const pipeline)
{
    sceneBuffer = nullptr;
    sorted.clear();
}

void MTransparentStage::preRender(IRenderPipeline* const pipeline) {}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------

void MTransparentStage::render(IRenderPipeline* const pipeline)
{
    if (!sceneBuffer || sceneBuffer->getFBOHandle() == 0) return;
    if (!(pipeline->getCompositeFlags() & ECF_Scene)) return;   // resolve didn't run

    const auto& items = pipeline->getTransparentItems();
    if (items.empty()) return;

    const SViewData viewData = resolveViewData(pipeline);
    if (!viewData.valid) return;

    // ---- Sort back to front -------------------------------------------------
    sorted.clear();
    for (const SRenderItem& item : items)
    {
        if (item.vao == 0 || !item.material || !item.material->getShader()) continue;
        if (!item.cameraVisible) continue;
        sorted.push_back(&item);
    }
    if (sorted.empty()) return;

    const glm::vec3 camPos = viewData.cameraPosition;
    auto sortPoint = [](const SRenderItem* item) -> glm::vec3
    {
        // Bounds centre; degenerate bounds mean "no bounds" - use the origin
        // of the item's transform instead.
        if (item->bounds.min == item->bounds.max)
            return glm::vec3(item->transform[3]);
        return (glm::vec3(item->bounds.min) + glm::vec3(item->bounds.max)) * 0.5f;
    };
    std::sort(sorted.begin(), sorted.end(),
              [&](const SRenderItem* a, const SRenderItem* b)
              {
                  const glm::vec3 da = sortPoint(a) - camPos;
                  const glm::vec3 db = sortPoint(b) - camPos;
                  return glm::dot(da, da) > glm::dot(db, db);   // farthest first
              });

    // ---- State --------------------------------------------------------------
    const SVector2 res = pipeline->getRenderResolution();
    sceneBuffer->bindAsActive();
    glViewport(0, 0, static_cast<int>(res.x), static_cast<int>(res.y));

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);    // hidden behind opaque geometry...
    glDepthMask(GL_FALSE);     // ...and by default hides nothing itself (Write Depth: per item below)
    glEnable(GL_BLEND);

    auto* renderer  = MEngineSubsystemRegistry::getSubsystem<IRenderPipelineManagerSubsystem>();
    auto* lights    = renderer ? &renderer->getLightSystem() : nullptr;
    const bool lightOverride = pipeline->getLightOverride().active;

    // Light override (thumbnails): ambient + directional only, no point/spot.
    if (lights && lightOverride)
        lights->prepareViewDynamicLights(nullptr, SVector3(0.f));

    SShaderPropertyValue viewVal, projVal;
    viewVal.setMat4Val(viewData.view);
    projVal.setMat4Val(viewData.projection);

    GLuint lastProgram     = 0;
    int    highestTexUnit  = -1;

    for (const SRenderItem* item : sorted)
    {
        MMaterial* material = item->material;
        MShader*   shader   = material->getShader();

        shader->bind();
        GLint progInt = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &progInt);
        const GLuint prog = static_cast<GLuint>(progInt);

        // Per-program setup only when the program changes.
        if (prog != lastProgram)
        {
            shader->setPropertyValue("view",       viewVal);
            shader->setPropertyValue("projection", projVal);

            const GLint camLoc = glGetUniformLocation(prog, "_cameraPos");
            if (camLoc >= 0) glUniform3fv(camLoc, 1, glm::value_ptr(viewData.cameraPosition));

            MShadowInputs::bind(pipeline, prog);
            lastProgram = prog;
        }

        SShaderPropertyValue modelVal;
        modelVal.setMat4Val(item->transform);
        shader->setPropertyValue("model", modelVal);

        const bool  lit    = item->getShadingMode() == MMaterial::ShadingMode::Lit;
        const GLint litLoc = glGetUniformLocation(prog, "_lit");
        if (litLoc >= 0)
            glUniform1i(litLoc, lit ? 1 : 0);

        highestTexUnit = std::max(highestTexUnit, bindMaterialProperties(*item, prog));

        // Point / spot lights near this mesh (BVH query).
        if (lights && !lightOverride)
            lights->prepareDynamicLights(item->bounds);

        // ---- Blend + culling per material ----------------------------------
        if (material->getBlendMode() == MMaterial::BlendMode::Additive)
        {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        }
        else
        {
            // Colour: standard "over". Alpha channel isn't read downstream,
            // keep it as accumulated coverage.
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                                GL_ONE,       GL_ONE_MINUS_SRC_ALPHA);
        }

        if (material->isDoubleSided()) glDisable(GL_CULL_FACE);
        else                           glEnable(GL_CULL_FACE);

        // Opt-in depth for post effects in the RS_PostProcessing slot.
        // Lighting and RS_PostOpaque have already run, so they never see it.
        glDepthMask(material->getWritesDepth() ? GL_TRUE : GL_FALSE);

        // ---- Draw ------------------------------------------------------------
        // instanceCount > 0: one call draws every instance (e.g. every
        // particle) — the VAO supplies the per-instance attributes.
        glBindVertexArray(item->vao);
        if (item->ebo != 0)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, item->ebo);
            if (item->instanceCount > 0)
                glDrawElementsInstanced(GL_TRIANGLES, item->indexCount, GL_UNSIGNED_INT, nullptr, item->instanceCount);
            else
                glDrawElements(GL_TRIANGLES, item->indexCount, GL_UNSIGNED_INT, nullptr);
        }
        else
        {
            if (item->instanceCount > 0)
                glDrawArraysInstanced(GL_TRIANGLES, 0, item->vertexCount, item->instanceCount);
            else
                glDrawArrays(GL_TRIANGLES, 0, item->vertexCount);
        }
    }

    // ---- Restore ------------------------------------------------------------
    glBindVertexArray(0);
    glUseProgram(0);

    glDisable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    for (int unit = MShadowInputs::UNIT_COUNT; unit <= highestTexUnit; ++unit)
    {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    MShadowInputs::unbind();

    sceneBuffer->unbind();
    pipeline->addCompositeFlag(ECF_Transparent);
}

int MTransparentStage::bindMaterialProperties(const SRenderItem& item, unsigned int program) const
{
    const GLuint prog   = program;
    MShader*     shader = item.material->getShader();

    int texUnit = MShadowInputs::UNIT_COUNT;
    int highest = -1;

    for (const auto& [key, val] : item.material->getProperties())
    {
        if (val.getType() != SShaderPropertyType::Texture)
        {
            // Colours are converted sRGB -> linear inside setPropertyValue.
            shader->setPropertyValue(key, val);
            continue;
        }

        // Each texture gets its own unit (MMaterial::bindMaterial puts every
        // texture on unit 0).
        const GLint loc = glGetUniformLocation(prog, key.c_str());
        if (loc < 0) continue;
        if (texUnit > MAX_TEX_UNIT) continue;

        // Empty or missing -> white, never a stale texture.
        bool bound = false;
        if (!val.getTexAssetReference().empty())
        {
            const auto texAsset = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>()
                ->getAssetFromReference<MTextureAsset>(val.getTexAssetReference());
            if (texAsset && texAsset->getTexture())
            {
                texAsset->bindForRendering(texUnit);
                bound = true;
            }
        }
        if (!bound) MDefaultTextures::bindWhite(texUnit);
        glUniform1i(loc, texUnit);
        highest = texUnit;
        ++texUnit;
    }
    return highest;
}

void MTransparentStage::postRender(IRenderPipeline* const pipeline) {}
