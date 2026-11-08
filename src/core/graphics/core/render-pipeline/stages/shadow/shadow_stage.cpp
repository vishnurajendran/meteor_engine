#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/camera/viewmanagement.h"
#include "core/engine/lighting/directional/directional_light.h"
#include "core/engine/lighting/dynamiclights/dynamic_light.h"
#include "core/graphics/core/render-pipeline/buffer_registry.h"
#include "core/graphics/core/render-pipeline/cutout_inputs.h"
#include "core/graphics/core/render-pipeline/buffers/buffer_names.h"
#include "core/graphics/core/render-pipeline/buffers/shadow/shadow_buffer.h"
#include "core/graphics/core/render-pipeline/interfaces/render_pipeline_interface.h"
#include "core/graphics/core/render-pipeline/render_item.h"
#include "core/graphics/core/render-pipeline/view_data.h"
#include "core/graphics/core/render-pipeline/stages/lighting/lighting_system_manager.h"
#include "core/graphics/core/render-pipeline/render_pipeline_manager_interface.h"
#include "core/graphics/core/shader/shader.h"
#include "core/graphics/core/shader/shaderasset.h"
#include "core/utils/frustum.h"
#include <algorithm>
#include <cmath>
#include "core/utils/logger.h"
#include "shadow_stage.h"

#include "core/engine/subsystem/subsystem_registry.h"

static constexpr const char* SHADOW_SHADER_PATH =
    "meteor_assets/engine_assets/shaders/internal/shadow_pass.mesl";

// ---------------------------------------------------------------------------
// Point light shadow shader - writes LINEAR distance / far as gl_FragDepth.
// This avoids the NDC-to-linear conversion problem entirely.
// Only used for the 6-face cube map pass, not directional/spot.
// ---------------------------------------------------------------------------
static const char* k_pointShadowVert = R"(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=2) in vec2 aUV;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec3 vWorldPos;
out vec2 vUV;
void main()
{
    vec4 worldPos   = model * vec4(aPos, 1.0);
    vWorldPos       = worldPos.xyz;
    vUV             = aUV;
    gl_Position     = projection * view * worldPos;
}
)";

static const char* k_pointShadowFrag = R"(
#version 330 core
in  vec3  vWorldPos;
in  vec2  vUV;
uniform vec3  u_lightPos;
uniform float u_farPlane;
// Alpha cutout (see cutout_inputs.h)
uniform int       _useCutout;
uniform float     _cutoutThreshold;
uniform int       _useAlbedoTex;
uniform sampler2D _albedo;
uniform vec4      _color;
void main()
{
    if (_useCutout == 1)
    {
        float alpha = _color.a;
        if (_useAlbedoTex == 1) alpha *= texture(_albedo, vUV).a;
        if (alpha < _cutoutThreshold) discard;
    }

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

// Per-light shadow culling — true if the item can land in this light's
// shadow map. Deliberately ignores item.cameraVisible: a caster outside the
// camera view can still throw a shadow into it.
static bool passesShadowCull(const SRenderItem& item, const SFrustum* frustum)
{
    if (!frustum) return true;
    if (item.bounds.min == item.bounds.max) return true; // no bounds — never cull
    return frustum->testAABB(item.bounds);
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
        MERROR("MShadowStage::init - failed to create shadow buffer");
        return;
    }

    // The base SDepthBuffer (single directional map) is no longer sampled -
    // directional shadows live in the cascade array. Keep it at 1x1 so the
    // buffer still reports a valid FBO to existing checks.
    shadowBuffer->resize(1, 1);
    shadowBuffer->initCascades(SShadowBuffer::CASCADE_RESOLUTION);
    shadowBuffer->initDynamicShadows(1024);

    const auto asset = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>()->getAsset<MShaderAsset>(SHADOW_SHADER_PATH);
    if (!asset)
        MERROR("MShadowStage::init - could not load: " + SString(SHADOW_SHADER_PATH));
    else
        shadowShader = asset->getShader();

    pointShadowProgram = compilePointShadowProgram();
}

void MShadowStage::cleanup(IRenderPipeline* const pipeline)
{
    if (shadowBuffer)
    {
        shadowBuffer->destroyDynamicShadows();
        shadowBuffer->destroyCascades();
    }
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

    auto* renderer = MEngineSubsystemRegistry::getSubsystem<IRenderPipelineManagerSubsystem>();
    if (!renderer) return;
    auto* mgr = &renderer->getLightSystem();

    shadowBuffer->numCascades = 0;   // stays 0 if the directional shadow is skipped
    ++frameIndex;   // ages cached spot / point shadow maps (rolling updates)

    auto* dirLight = mgr->getDirectionalLight();
    if (dirLight && dirLight->getEnabled() && mgr->isDirectionalShadowEnabled())
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
    if (shadowBuffer->cascadeFBO == 0) return;

    // Same camera as every other stage (respects the pipeline camera override).
    const SViewData viewData = resolveViewData(pipeline);
    if (!viewData.valid) return;

    const auto* dir        = dynamic_cast<MDirectionalLight*>(dirLight);
    const float distance   = dir ? dir->getShadowDistance() : 100.0f;
    const int   count      = dir ? glm::clamp(dir->getCascadeCount(), 1, SShadowBuffer::MAX_CASCADES)
                                 : SShadowBuffer::MAX_CASCADES;
    const float splitBlend = dir ? dir->getCascadeSplit() : 0.75f;

    const glm::vec3 towardLight = glm::normalize(dirLight->getForwardVector());
    const glm::vec3 up = (glm::abs(glm::dot(towardLight, glm::vec3(0,1,0))) < 0.99f)
                          ? glm::vec3(0,1,0) : glm::vec3(1,0,0);

    // ---- Camera frustum corners, view space -----------------------------------
    // Near corners at NDC z = -1, far at z = +1. A point at view distance d
    // lies at the same fraction along each near->far edge (view z is linear
    // along the edge), so a slice of the frustum is a lerp of these corners.
    // Works for perspective and orthographic cameras alike.
    glm::vec3 nearCorner[4], farCorner[4];
    {
        const glm::vec2 ndc[4] = { {-1,-1}, {1,-1}, {1,1}, {-1,1} };
        for (int i = 0; i < 4; ++i)
        {
            glm::vec4 n = viewData.inverseProjection * glm::vec4(ndc[i], -1.f, 1.f);
            glm::vec4 f = viewData.inverseProjection * glm::vec4(ndc[i],  1.f, 1.f);
            nearCorner[i] = glm::vec3(n) / n.w;
            farCorner[i]  = glm::vec3(f) / f.w;
        }
    }
    const float camNear = -nearCorner[0].z;
    const float camFar  = -farCorner[0].z;
    if (camFar <= camNear) return;

    const float shadowFar = glm::clamp(distance, camNear + 0.01f, camFar);

    // ---- Split distances ---------------------------------------------------------
    // Blend of logarithmic (resolution concentrated near the camera) and
    // uniform splits - the usual "practical split scheme".
    float splits[SShadowBuffer::MAX_CASCADES] = {};
    for (int i = 0; i < count; ++i)
    {
        const float p      = static_cast<float>(i + 1) / static_cast<float>(count);
        const float logD   = camNear * glm::pow(shadowFar / camNear, p);
        const float linD   = camNear + (shadowFar - camNear) * p;
        splits[i] = glm::mix(linD, logD, splitBlend);
    }
    splits[count - 1] = shadowFar;

    const float res = static_cast<float>(SShadowBuffer::CASCADE_RESOLUTION);

    glBindFramebuffer(GL_FRAMEBUFFER, shadowBuffer->cascadeFBO);
    glViewport(0, 0, SShadowBuffer::CASCADE_RESOLUTION, SShadowBuffer::CASCADE_RESOLUTION);
    setGLDepthOnlyState();

    shadowBuffer->cascadeCameraView = viewData.view;
    shadowBuffer->shadowDistance    = shadowFar;

    float sliceStart = camNear;
    for (int c = 0; c < count; ++c)
    {
        const float sliceEnd = splits[c];

        // ---- Slice corners -> world space ---------------------------------------
        const float t0 = (sliceStart - camNear) / (camFar - camNear);
        const float t1 = (sliceEnd   - camNear) / (camFar - camNear);
        glm::vec3 corners[8];
        for (int i = 0; i < 4; ++i)
        {
            const glm::vec3 a = glm::mix(nearCorner[i], farCorner[i], t0);
            const glm::vec3 b = glm::mix(nearCorner[i], farCorner[i], t1);
            corners[i]     = glm::vec3(viewData.inverseView * glm::vec4(a, 1.f));
            corners[i + 4] = glm::vec3(viewData.inverseView * glm::vec4(b, 1.f));
        }

        // ---- Bounding sphere ----------------------------------------------------
        // A sphere doesn't change size when the camera rotates, so the ortho
        // box (and texel size) stays constant - no shimmering on rotation.
        glm::vec3 center(0.f);
        for (const auto& p : corners) center += p;
        center /= 8.f;
        float radius = 0.f;
        for (const auto& p : corners) radius = glm::max(radius, glm::length(p - center));
        radius = glm::ceil(radius * 16.f) / 16.f;   // quantise against float jitter

        // Pull the light back to catch casters outside the slice (tall
        // objects, things behind the camera) that throw shadows into it.
        const float     margin   = glm::max(radius, 100.0f);
        const glm::vec3 lightPos = center + towardLight * (radius + margin);

        const glm::mat4 lightView = glm::lookAt(lightPos, center, up);
        glm::mat4       lightProj = glm::ortho(-radius, radius, -radius, radius,
                                               0.0f, 2.0f * (radius + margin));

        // Snap to whole texels so the texel grid doesn't slide when the
        // camera moves - otherwise shadow edges crawl.
        {
            glm::vec4 origin = lightProj * lightView * glm::vec4(0, 0, 0, 1);
            origin *= res * 0.5f;
            const glm::vec2 offset = (glm::round(glm::vec2(origin)) - glm::vec2(origin)) * (2.0f / res);
            lightProj[3][0] += offset.x;
            lightProj[3][1] += offset.y;
        }

        const glm::mat4 lightVP = lightProj * lightView;
        shadowBuffer->cascadeMatrices[c]   = lightVP;
        shadowBuffer->cascadeSplits[c]     = sliceEnd;
        shadowBuffer->cascadeTexelWorld[c] = (2.0f * radius) / res;

        // ---- Render this cascade's layer ---------------------------------------
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                  shadowBuffer->cascadeArrayTexture, 0, c);
        glClear(GL_DEPTH_BUFFER_BIT);

        shadowShader->bind();
        SShaderPropertyValue v, p;
        v.setMat4Val(lightView); p.setMat4Val(lightProj);
        shadowShader->setPropertyValue("view",       v);
        shadowShader->setPropertyValue("projection", p);

        // Cull against this cascade's ortho box - anything inside can cast
        // into it, whether or not the camera sees it.
        SFrustum cascadeFrustum;
        cascadeFrustum.extractFromVP(lightVP);
        drawItems(pipeline, shadowShader, nullptr, true, &cascadeFrustum);

        sliceStart = sliceEnd;
    }

    shadowBuffer->numCascades = count;

    restoreGLDepthOnlyState();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    pipeline->addCompositeFlag(ECF_Shadow);
}

// ---------------------------------------------------------------------------
// Shadow caster selection (spot / point)
// ---------------------------------------------------------------------------

std::vector<MDynamicLight*> MShadowStage::selectShadowCasters(IRenderPipeline* pipeline, ELightType type,
                                                              int slots, SShadowSelection& state)
{
    std::vector<MDynamicLight*> selected;
    auto* renderer = MEngineSubsystemRegistry::getSubsystem<IRenderPipelineManagerSubsystem>();
    if (!renderer) return selected;
    const auto& dynLights = renderer->getLightSystem().getDynamicLights();

    // Start from "no shadow" for every light of this type: a light that had a
    // slot last frame must never keep pointing at a map now used by another.
    for (auto* light : dynLights)
        if (light && light->getLightType() == type)
            light->setShadowIndex(-1);

    // Camera frustum: a light whose range sphere is entirely off-screen can't
    // shadow anything visible.
    const SViewData view = resolveViewData(pipeline);
    SFrustum cameraFrustum;
    if (view.valid) cameraFrustum.extractFromVP(view.projection * view.view);

    struct SCandidate { MDynamicLight* light; int priority; float score; };
    std::vector<SCandidate> candidates;

    for (auto* light : dynLights)
    {
        if (!light || light->getLightType() != type) continue;
        if (!light->getEnabled() || !light->getCastsShadow()) continue;

        const glm::vec3 pos   = glm::vec3(light->getWorldPosition());
        const float     range = light->getRange();
        if (view.valid && !cameraFrustum.testAABB({ pos - glm::vec3(range), pos + glm::vec3(range) }))
            continue;

        // Distance from the camera to the light's range sphere: 0 when the
        // camera is inside it. Centre distance breaks ties between lights the
        // camera is inside. Last frame's lights get a 20% head start.
        const float centre = glm::length(pos - view.cameraPosition);
        float score = std::max(centre - range, 0.0f) + centre * 0.001f;
        if (state.lastSelected.contains(light)) score *= 0.8f;

        candidates.push_back({ light, light->getShadowPriority(), score });
    }

    std::sort(candidates.begin(), candidates.end(), [](const SCandidate& a, const SCandidate& b)
    {
        if (a.priority != b.priority) return a.priority > b.priority;
        return a.score < b.score;
    });

    const int count = std::min(static_cast<int>(candidates.size()), slots);
    state.lastSelected.clear();
    for (int i = 0; i < count; ++i)
    {
        selected.push_back(candidates[i].light);
        state.lastSelected.insert(candidates[i].light);
    }

    // Log only when the number of visible lights going without shadows changes.
    const int skipped = static_cast<int>(candidates.size()) - count;
    if (skipped != state.lastSkipped)
    {
        if (skipped > 0)
            MLOG(SString::format("MShadowStage: {0} {1} light(s) in view without shadows ({2} slots) - "
                                 "highest priority / nearest lights get them",
                                 SString::fromInt(skipped),
                                 SString(type == ELightType::Spot ? "spot" : "point"),
                                 SString::fromInt(slots)));
        state.lastSkipped = skipped;
    }
    return selected;
}

// ---------------------------------------------------------------------------
// Rolling slot updates (spot / point)
// ---------------------------------------------------------------------------

void MShadowStage::planSlotUpdates(const std::vector<MDynamicLight*>& selected,
                                   SShadowSlot* slots, int slotCount, int budget,
                                   std::vector<std::pair<MDynamicLight*, int>>& outRender)
{
    outRender.clear();

    auto owns = [](const SShadowSlot& s, const MDynamicLight* light)
    {
        // GUID too: a new light can be allocated at a deleted light's address.
        return s.owner == light && s.ownerGuid == light->getGUID();
    };

    // 1. Free slots whose owner is no longer selected.
    for (int i = 0; i < slotCount; ++i)
    {
        if (!slots[i].owner) continue;
        const bool stillSelected = std::ranges::any_of(selected, [&](const MDynamicLight* l) { return owns(slots[i], l); });
        if (!stillSelected) slots[i] = SShadowSlot{};
    }

    // 2. Selected lights keep their slot; new ones take a free one (no map yet).
    std::vector<int> slotOf(selected.size(), -1);
    for (size_t li = 0; li < selected.size(); ++li)
        for (int i = 0; i < slotCount; ++i)
            if (owns(slots[i], selected[li])) { slotOf[li] = i; break; }

    for (size_t li = 0; li < selected.size(); ++li)
    {
        if (slotOf[li] >= 0) continue;
        for (int i = 0; i < slotCount; ++i)
        {
            if (slots[i].owner) continue;
            slots[i]           = SShadowSlot{};
            slots[i].owner     = selected[li];
            slots[i].ownerGuid = selected[li]->getGUID();
            slotOf[li]         = i;
            break;
        }
    }

    // 3. Rank what needs rendering:
    //    0 - no map yet (new owner)
    //    1 - the light moved / turned / changed range or angle, or it is the
    //        top-ranked light (kept live: usually the one nearest the camera;
    //        only with a budget above 1)
    //    2 - everything else, oldest map first (catches moving objects)
    struct SNeed { int urgency; uint64_t age; MDynamicLight* light; int slot; };
    std::vector<SNeed> needs;
    for (size_t li = 0; li < selected.size(); ++li)
    {
        const int i = slotOf[li];
        if (i < 0) continue;
        MDynamicLight*   light = selected[li];
        const SShadowSlot& s   = slots[i];

        const auto data   = light->getLightData();
        const glm::vec3 p = glm::vec3(light->getWorldPosition());
        const glm::vec3 d = glm::normalize(-light->getForwardVector());
        const bool changed = glm::length(p - s.position) > 0.001f
                          || glm::dot(d, s.direction) < 0.9999f
                          || std::abs(data.range - s.range) > 1e-4f
                          || std::abs(data.angle - s.angle) > 1e-4f;

        // The top-ranked light is kept live only when the budget leaves room
        // for others; with a budget of 1 it would starve the rest.
        const bool keepLive = (li == 0 && budget > 1);

        int urgency = 2;
        if (!s.valid)                   urgency = 0;
        else if (changed || keepLive)   urgency = 1;
        needs.push_back({ urgency, frameIndex - s.lastRendered, light, i });
    }

    std::sort(needs.begin(), needs.end(), [](const SNeed& a, const SNeed& b)
    {
        if (a.urgency != b.urgency) return a.urgency < b.urgency;
        return a.age > b.age;
    });

    const int n = std::min(static_cast<int>(needs.size()), std::max(budget, 1));
    for (int k = 0; k < n; ++k)
        outRender.emplace_back(needs[k].light, needs[k].slot);

    // 4. Every owner with a rendered map uses it; owners still waiting for
    //    their first render go without a shadow (never a wrong / empty map).
    //    The shadow index is set again after rendering for this frame's renders.
    for (size_t li = 0; li < selected.size(); ++li)
    {
        const int i = slotOf[li];
        selected[li]->setShadowIndex((i >= 0 && slots[i].valid) ? i : -1);
    }
}

// Remembers what a slot was rendered with, for change detection.
static void recordSlot(MShadowStage::SShadowSlot& s, const MDynamicLight* light, uint64_t frame)
{
    const auto data = light->getLightData();
    s.valid        = true;
    s.position     = glm::vec3(light->getWorldPosition());
    s.direction    = glm::normalize(-light->getForwardVector());
    s.range        = data.range;
    s.angle        = data.angle;
    s.lastRendered = frame;
}

// Highest slot in use + 1: maps above it are never sampled.
static int usedSlotCount(const MShadowStage::SShadowSlot* slots, int count)
{
    int used = 0;
    for (int i = 0; i < count; ++i)
        if (slots[i].owner && slots[i].valid) used = i + 1;
    return used;
}

// ---------------------------------------------------------------------------
// Spot lights
// ---------------------------------------------------------------------------

void MShadowStage::renderSpotShadows(IRenderPipeline* const pipeline)
{
    // Best lights first; every other spot light is left without a shadow.
    const auto casters = selectShadowCasters(pipeline, ELightType::Spot,
                                             SShadowBuffer::MAX_SPOT_SHADOWS, spotSelection);

    std::vector<std::pair<MDynamicLight*, int>> toRender;
    planSlotUpdates(casters, spotSlots, SShadowBuffer::MAX_SPOT_SHADOWS, SPOT_UPDATES_PER_FRAME, toRender);

    for (const auto& [light, slot] : toRender)
    {
        const glm::vec3 lightPos  = glm::vec3(light->getWorldPosition());
        // getForwardVector() points AWAY from the shine direction (gizmo uses -forward).
        // Negate to get the actual direction the spot light illuminates.
        const glm::vec3 shineDir  = glm::normalize(-light->getForwardVector());
        const glm::vec3 up        = (glm::abs(glm::dot(shineDir, glm::vec3(0,1,0))) < 0.99f)
                                     ? glm::vec3(0,1,0) : glm::vec3(1,0,0);

        const glm::mat4 lightView = glm::lookAt(lightPos, lightPos + shineDir, up);
        const float fov      = glm::degrees(light->getLightData().angle) * 2.0f;
        const float farPlane = light->getLightData().range;
        const glm::mat4 lightProj = glm::perspective(glm::radians(fov), 1.0f, 0.1f, farPlane);

        shadowBuffer->spotLightSpaceMatrices[slot] = lightProj * lightView;
        shadowBuffer->spotLightPositions[slot]     = lightPos;
        shadowBuffer->spotFarPlanes[slot]          = farPlane;

        // Use the linear depth program (same as point lights) so the stored
        // depth is distance/far - trivially comparable without NDC conversion.
        glBindFramebuffer(GL_FRAMEBUFFER, shadowBuffer->spotFBOs[slot]);
        glViewport(0, 0, 1024, 1024);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);
        glDisable(GL_CULL_FACE); // disable culling for robustness
        glEnable(GL_POLYGON_OFFSET_FILL);
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
        // Cull against the spot's perspective frustum (cone bounded by range).
        SFrustum spotFrustum;
        spotFrustum.extractFromVP(lightProj * lightView);
        drawItemsRaw(pipeline, pointShadowProgram, true, &spotFrustum);

        glDisable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(0.f, 0.f);
        glEnable(GL_CULL_FACE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glUseProgram(0);

        recordSlot(spotSlots[slot], light, frameIndex);
        light->setShadowIndex(slot);
    }

    shadowBuffer->numSpotShadows = usedSlotCount(spotSlots, SShadowBuffer::MAX_SPOT_SHADOWS);
    if (shadowBuffer->numSpotShadows > 0) pipeline->addCompositeFlag(ECF_Shadow);
}

// ---------------------------------------------------------------------------
// Point lights - linear depth cube map
// ---------------------------------------------------------------------------

void MShadowStage::renderPointShadows(IRenderPipeline* const pipeline)
{
    if (!pointShadowProgram) return;

    // Best lights first; every other point light is left without a shadow.
    const auto casters = selectShadowCasters(pipeline, ELightType::Point,
                                             SShadowBuffer::MAX_POINT_SHADOWS, pointSelection);

    std::vector<std::pair<MDynamicLight*, int>> toRender;
    planSlotUpdates(casters, pointSlots, SShadowBuffer::MAX_POINT_SHADOWS, POINT_UPDATES_PER_FRAME, toRender);

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

    for (const auto& [light, slot] : toRender)
    {
        const glm::vec3 lightPos = glm::vec3(light->getWorldPosition());
        const float     farPlane = light->getLightData().range;
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
            glDisable(GL_CULL_FACE); // omnidirectional - no culling
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(1.1f, 2.0f);
            glClear(GL_DEPTH_BUFFER_BIT);

            const glm::mat4 faceView = glm::lookAt(lightPos, lightPos + dirs[face], ups[face]);
            GLint viewLoc = glGetUniformLocation(pointShadowProgram, "view");
            if (viewLoc >= 0)
                glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(faceView));

            // Cull per cube face — each face is a 90° frustum out to range,
            // so an item is only drawn into the faces it can actually touch.
            SFrustum faceFrustum;
            faceFrustum.extractFromVP(proj * faceView);
            drawItemsRaw(pipeline, pointShadowProgram, true, &faceFrustum);

            glDisable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(0.f, 0.f);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glUseProgram(0);
        glEnable(GL_CULL_FACE);

        recordSlot(pointSlots[slot], light, frameIndex);
        light->setShadowIndex(slot);
    }

    shadowBuffer->numPointShadows = usedSlotCount(pointSlots, SShadowBuffer::MAX_POINT_SHADOWS);
    if (shadowBuffer->numPointShadows > 0) pipeline->addCompositeFlag(ECF_Shadow);
}

// ---------------------------------------------------------------------------
// Draw helpers
// ---------------------------------------------------------------------------

void MShadowStage::drawItems(IRenderPipeline* const pipeline,
                             MShader* shader, unsigned int* rawProg,
                             bool shadowCastersOnly, const SFrustum* cullFrustum)
{
    // The shader is bound by the caller; cutout uniforms go straight to it.
    GLint prog = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prog);

    // Transparents don't cast shadows (yet) - opaque items only.
    for (const SRenderItem& item : pipeline->getOpaqueItems())
    {
        if (item.vao == 0) continue;
        if (shadowCastersOnly && !item.castsShadow) continue;
        if (!passesShadowCull(item, cullFrustum)) continue;

        // Directional pass culls back faces; double-sided casters draw both.
        // (Spot/point passes go through drawItemsRaw with culling off.)
        if (item.isDoubleSided()) glDisable(GL_CULL_FACE);
        else                      glEnable(GL_CULL_FACE);

        SShaderPropertyValue modelVal;
        modelVal.setMat4Val(item.transform);
        shader->setPropertyValue("model", modelVal);
        bindCutoutInputs(item, static_cast<unsigned int>(prog), 0);

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
    glEnable(GL_CULL_FACE);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);   // cutout albedo
}

void MShadowStage::drawItemsRaw(IRenderPipeline* const pipeline,
                                unsigned int prog, bool shadowCastersOnly,
                                const SFrustum* cullFrustum)
{
    GLint modelLoc = glGetUniformLocation(prog, "model");
    for (const SRenderItem& item : pipeline->getOpaqueItems())
    {
        if (item.vao == 0) continue;
        if (shadowCastersOnly && !item.castsShadow) continue;
        if (!passesShadowCull(item, cullFrustum)) continue;

        if (modelLoc >= 0)
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(item.transform));
        bindCutoutInputs(item, prog, 0);

        glBindVertexArray(item.vao);
        if (item.ebo != 0)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, item.ebo);
            glDrawElements(GL_TRIANGLES, item.indexCount, GL_UNSIGNED_INT, nullptr);
        }
        else glDrawArrays(GL_TRIANGLES, 0, item.vertexCount);
    }
    glBindVertexArray(0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);   // cutout albedo
}