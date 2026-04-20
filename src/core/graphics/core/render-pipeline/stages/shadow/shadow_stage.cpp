#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/camera/viewmanagement.h"
#include "core/engine/lighting/dynamiclights/dynamic_light.h"
#include "core/graphics/core/render-pipeline/buffer_registry.h"
#include "core/graphics/core/render-pipeline/buffers/buffer_names.h"
#include "core/graphics/core/render-pipeline/buffers/shadow/shadow_buffer.h"
#include "core/graphics/core/render-pipeline/interfaces/render_pipeline_interface.h"
#include "core/graphics/core/render-pipeline/render_item.h"
#include "core/graphics/core/render-pipeline/stages/lighting/lighting_system_manager.h"
#include "core/graphics/core/shader/shader.h"
#include "core/graphics/core/shader/shaderasset.h"
#include "core/utils/logger.h"
#include "shadow_stage.h"

static constexpr const char* SHADOW_SHADER_PATH =
    "meteor_assets/engine_assets/shaders/shadow_pass.mesl";

// ---------------------------------------------------------------------------
// Point light shadow shader — writes LINEAR distance / far as gl_FragDepth.
// This avoids the NDC-to-linear conversion problem entirely.
// Only used for the 6-face cube map pass, not directional/spot.
// ---------------------------------------------------------------------------
static const char* k_pointShadowVert = R"(
#version 330 core
layout(location=0) in vec3 aPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec3 vWorldPos;
void main()
{
    vec4 worldPos   = model * vec4(aPos, 1.0);
    vWorldPos       = worldPos.xyz;
    gl_Position     = projection * view * worldPos;
}
)";

static const char* k_pointShadowFrag = R"(
#version 330 core
in  vec3  vWorldPos;
uniform vec3  u_lightPos;
uniform float u_farPlane;
void main()
{
    // Write normalised linear distance so sampling is trivial:
    //   storedDepth * farPlane = actual distance from light.
    float dist = length(vWorldPos - u_lightPos);
    gl_FragDepth = dist / u_farPlane;
}
)";

static unsigned int compilePointShadowProgram()
{
    auto compile = [](GLenum type, const char* src) -> unsigned int {
        unsigned int s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[512]; glGetShaderInfoLog(s, 512, nullptr, log);
            MERROR(SString("MShadowStage point shader: ") + log);
        }
        return s;
    };
    unsigned int vert = compile(GL_VERTEX_SHADER,   k_pointShadowVert);
    unsigned int frag = compile(GL_FRAGMENT_SHADER, k_pointShadowFrag);
    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vert); glAttachShader(prog, frag);
    glLinkProgram(prog);
    glDeleteShader(vert); glDeleteShader(frag);
    return prog;
}

static void setGLDepthOnlyState()
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0f, 4.0f); // stronger offset reduces shadow acne artifacts
}

static void restoreGLDepthOnlyState()
{
    glDisable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(0.f, 0.f);
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void MShadowStage::init(IRenderPipeline* const pipeline)
{
    shadowBuffer = pipeline->getBufferRegistry()
                            .createBuffer<SShadowBuffer>(MBufferNames::BUFFER_SHADOW);
    if (!shadowBuffer)
    {
        MERROR("MShadowStage::init — failed to create shadow buffer");
        return;
    }

    shadowBuffer->resize(SShadowBuffer::SHADOW_MAP_RESOLUTION,
                         SShadowBuffer::SHADOW_MAP_RESOLUTION);
    shadowBuffer->initDynamicShadows(1024);

    auto* asset = MAssetManager::getInstance()->getAsset<MShaderAsset>(SHADOW_SHADER_PATH);
    if (!asset)
        MERROR("MShadowStage::init — could not load: " + SString(SHADOW_SHADER_PATH));
    else
        shadowShader = asset->getShader();

    pointShadowProgram = compilePointShadowProgram();
}

void MShadowStage::cleanup(IRenderPipeline* const pipeline)
{
    if (shadowBuffer) shadowBuffer->destroyDynamicShadows();
    shadowBuffer = nullptr;
    shadowShader = nullptr;
    if (pointShadowProgram) { glDeleteProgram(pointShadowProgram); pointShadowProgram = 0; }
}

void MShadowStage::preRender (IRenderPipeline* const pipeline) {}
void MShadowStage::postRender(IRenderPipeline* const pipeline) {}

// ---------------------------------------------------------------------------
// render()
// ---------------------------------------------------------------------------

void MShadowStage::render(IRenderPipeline* const pipeline)
{
    if (!shadowBuffer || !shadowShader) return;

    auto* mgr = MLightSystemManager::getInstance();

    auto* dirLight = mgr->getDirectionalLight();
    if (dirLight && dirLight->getEnabled() && mgr->directionalShadowEnabled)
        renderDirectionalShadow(pipeline, dirLight);

    renderSpotShadows(pipeline);
    renderPointShadows(pipeline);
}

// ---------------------------------------------------------------------------
// Directional
// ---------------------------------------------------------------------------

void MShadowStage::renderDirectionalShadow(IRenderPipeline* const pipeline,
                                           MLightEntity* dirLight)
{
    const glm::vec3 towardLight = glm::normalize(dirLight->getForwardVector());

    MCameraEntity* camera = nullptr;
    for (auto* c : MViewManagement::getCameras())
        if (c && c->getEnabled()) { camera = c; break; }

    const glm::vec3 anchor   = camera ? glm::vec3(camera->getWorldPosition()) : glm::vec3(0.f);
    // Fixed shadow distance. At 1 unit = 1 metre this gives 50m coverage —
    // a generous range for a typical scene without wasting shadow map resolution.
    constexpr float pullback = 120.0f;
    const glm::vec3 lightPos = anchor + towardLight * pullback;

    const glm::vec3 up = (glm::abs(glm::dot(towardLight, glm::vec3(0,1,0))) < 0.99f)
                          ? glm::vec3(0,1,0) : glm::vec3(1,0,0);

    const glm::mat4 lightView = glm::lookAt(lightPos, anchor, up);
    constexpr float half      = 80.0f * 0.5f;
    const glm::mat4 lightProj = glm::ortho(-half, half, -half, half,
                                            0.1f, 80.0f * 2.f + 80.0f);

    shadowBuffer->lightSpaceMatrix = lightProj * lightView;

    glBindFramebuffer(GL_FRAMEBUFFER, shadowBuffer->getFBOHandle());
    glViewport(0, 0, SShadowBuffer::SHADOW_MAP_RESOLUTION, SShadowBuffer::SHADOW_MAP_RESOLUTION);
    setGLDepthOnlyState();
    glClear(GL_DEPTH_BUFFER_BIT);

    shadowShader->bind();
    SShaderPropertyValue v, p;
    v.setMat4Val(lightView); p.setMat4Val(lightProj);
    shadowShader->setPropertyValue("view",       v);
    shadowShader->setPropertyValue("projection", p);
    drawItems(pipeline, shadowShader, nullptr, true);

    restoreGLDepthOnlyState();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    pipeline->addCompositeFlag(ECF_Shadow);
}

// ---------------------------------------------------------------------------
// Spot lights
// ---------------------------------------------------------------------------

void MShadowStage::renderSpotShadows(IRenderPipeline* const pipeline)
{
    const auto& dynLights = MLightSystemManager::getInstance()->getDynamicLights();
    int slot = 0;

    // Count spot lights for diagnostics.
    int spotCount = 0;
    for (auto* l : dynLights)
        if (l && l->getLightType() == ELightType::Spot) spotCount++;
    if (spotCount > 0 && spotCount > SShadowBuffer::MAX_SPOT_SHADOWS)
        MWARN("MShadowStage: more spot lights than shadow slots — some won't cast shadows");

    for (auto* light : dynLights)
    {
        if (slot >= SShadowBuffer::MAX_SPOT_SHADOWS) break;
        if (!light || !light->getEnabled()) continue;
        if (light->getLightType() != ELightType::Spot) continue;
        if (!light->getCastsShadow()) { light->setShadowIndex(-1); continue; }

        const glm::vec3 lightPos  = glm::vec3(light->getWorldPosition());
        // getForwardVector() points AWAY from the shine direction (gizmo uses -forward).
        // Negate to get the actual direction the spot light illuminates.
        const glm::vec3 shineDir  = glm::normalize(-light->getForwardVector());
        const glm::vec3 up        = (glm::abs(glm::dot(shineDir, glm::vec3(0,1,0))) < 0.99f)
                                     ? glm::vec3(0,1,0) : glm::vec3(1,0,0);

        const glm::mat4 lightView = glm::lookAt(lightPos, lightPos + shineDir, up);
        const auto* dyn = static_cast<MDynamicLight*>(light);
        const float fov      = glm::degrees(dyn->getLightData().angle) * 2.0f;
        const float farPlane = dyn->getLightData().range;
        const glm::mat4 lightProj = glm::perspective(glm::radians(fov), 1.0f, 0.1f, farPlane);

        shadowBuffer->spotLightSpaceMatrices[slot] = lightProj * lightView;
        shadowBuffer->spotLightPositions[slot]     = lightPos;
        shadowBuffer->spotFarPlanes[slot]          = farPlane;

        // Use the linear depth program (same as point lights) so the stored
        // depth is distance/far — trivially comparable without NDC conversion.
        glBindFramebuffer(GL_FRAMEBUFFER, shadowBuffer->spotFBOs[slot]);
        glViewport(0, 0, 1024, 1024);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);
        glDisable(GL_CULL_FACE); // disable culling for robustness
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.1f, 2.0f);
        glClear(GL_DEPTH_BUFFER_BIT);

        glUseProgram(pointShadowProgram);
        // Stronger polygon offset for spot: perspective projection at shallow
        // angles has poor depth precision, needs more offset than directional.
        glPolygonOffset(4.0f, 8.0f);
        {
            GLint lposLoc = glGetUniformLocation(pointShadowProgram, "u_lightPos");
            GLint farLoc  = glGetUniformLocation(pointShadowProgram, "u_farPlane");
            GLint projLoc = glGetUniformLocation(pointShadowProgram, "projection");
            GLint viewLoc = glGetUniformLocation(pointShadowProgram, "view");
            if (lposLoc >= 0) glUniform3fv(lposLoc, 1, glm::value_ptr(lightPos));
            if (farLoc  >= 0) glUniform1f(farLoc, farPlane);
            if (projLoc >= 0) glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(lightProj));
            if (viewLoc >= 0) glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(lightView));
        }
        drawItemsRaw(pipeline, pointShadowProgram, true);

        glDisable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(0.f, 0.f);
        glEnable(GL_CULL_FACE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glUseProgram(0);

        light->setShadowIndex(slot);
        ++slot;
    }

    shadowBuffer->numSpotShadows = slot;
    if (slot > 0) pipeline->addCompositeFlag(ECF_Shadow);

    for (auto* light : dynLights)
        if (light && light->getLightType() == ELightType::Spot && light->getShadowIndex() >= slot)
            light->setShadowIndex(-1);
}

// ---------------------------------------------------------------------------
// Point lights — linear depth cube map
// ---------------------------------------------------------------------------

void MShadowStage::renderPointShadows(IRenderPipeline* const pipeline)
{
    if (!pointShadowProgram) return;

    const auto& dynLights = MLightSystemManager::getInstance()->getDynamicLights();
    int slot = 0;

    static const glm::vec3 dirs[6] = {
        { 1, 0, 0},{-1, 0, 0},
        { 0, 1, 0},{ 0,-1, 0},
        { 0, 0, 1},{ 0, 0,-1}
    };
    static const glm::vec3 ups[6] = {
        { 0,-1, 0},{ 0,-1, 0},
        { 0, 0, 1},{ 0, 0,-1},
        { 0,-1, 0},{ 0,-1, 0}
    };

    for (auto* light : dynLights)
    {
        if (slot >= SShadowBuffer::MAX_POINT_SHADOWS) break;
        if (!light || !light->getEnabled()) continue;
        if (light->getLightType() != ELightType::Point) continue;
        if (!light->getCastsShadow()) { light->setShadowIndex(-1); continue; }

        const auto*     dyn      = static_cast<MDynamicLight*>(light);
        const glm::vec3 lightPos = glm::vec3(light->getWorldPosition());
        const float     farPlane = dyn->getLightData().range;
        const glm::mat4 proj     = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, farPlane);

        shadowBuffer->pointLightPositions[slot] = lightPos;
        shadowBuffer->pointFarPlanes[slot]      = farPlane;

        glUseProgram(pointShadowProgram);
        GLint lposLoc  = glGetUniformLocation(pointShadowProgram, "u_lightPos");
        GLint farLoc   = glGetUniformLocation(pointShadowProgram, "u_farPlane");
        GLint projLoc  = glGetUniformLocation(pointShadowProgram, "projection");
        if (lposLoc >= 0) glUniform3fv(lposLoc, 1, glm::value_ptr(lightPos));
        if (farLoc  >= 0) glUniform1f(farLoc, farPlane);
        if (projLoc >= 0) glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));

        for (int face = 0; face < 6; ++face)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, shadowBuffer->pointFBOs[slot]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                   GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                                   shadowBuffer->pointCubemapTextures[slot], 0);
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);

            glViewport(0, 0, 1024, 1024);
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);
            glDepthMask(GL_TRUE);
            glDisable(GL_CULL_FACE); // omnidirectional — no culling
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(1.1f, 2.0f);
            glClear(GL_DEPTH_BUFFER_BIT);

            const glm::mat4 faceView = glm::lookAt(lightPos, lightPos + dirs[face], ups[face]);
            GLint viewLoc = glGetUniformLocation(pointShadowProgram, "view");
            if (viewLoc >= 0)
                glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(faceView));

            drawItemsRaw(pipeline, pointShadowProgram, true);

            glDisable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(0.f, 0.f);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glUseProgram(0);
        glEnable(GL_CULL_FACE);

        light->setShadowIndex(slot);
        ++slot;
    }

    shadowBuffer->numPointShadows = slot;
    if (slot > 0) pipeline->addCompositeFlag(ECF_Shadow);
}

// ---------------------------------------------------------------------------
// Draw helpers
// ---------------------------------------------------------------------------

void MShadowStage::drawItems(IRenderPipeline* const pipeline,
                             MShader* shader, unsigned int* rawProg,
                             bool shadowCastersOnly)
{
    for (const SRenderItem& item : pipeline->getRenderItems())
    {
        if (item.vao == 0) continue;
        if (shadowCastersOnly && !item.castsShadow) continue;

        SShaderPropertyValue modelVal;
        modelVal.setMat4Val(item.transform);
        shader->setPropertyValue("model", modelVal);

        glBindVertexArray(item.vao);
        if (item.ebo != 0)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, item.ebo);
            glDrawElements(GL_TRIANGLES, item.indexCount, GL_UNSIGNED_INT, nullptr);
        }
        else glDrawArrays(GL_TRIANGLES, 0, item.vertexCount);
    }
    glBindVertexArray(0);
    glUseProgram(0);
}

void MShadowStage::drawItemsRaw(IRenderPipeline* const pipeline,
                                unsigned int prog, bool shadowCastersOnly)
{
    GLint modelLoc = glGetUniformLocation(prog, "model");
    for (const SRenderItem& item : pipeline->getRenderItems())
    {
        if (item.vao == 0) continue;
        if (shadowCastersOnly && !item.castsShadow) continue;

        if (modelLoc >= 0)
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(item.transform));

        glBindVertexArray(item.vao);
        if (item.ebo != 0)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, item.ebo);
            glDrawElements(GL_TRIANGLES, item.indexCount, GL_UNSIGNED_INT, nullptr);
        }
        else glDrawArrays(GL_TRIANGLES, 0, item.vertexCount);
    }
    glBindVertexArray(0);
}