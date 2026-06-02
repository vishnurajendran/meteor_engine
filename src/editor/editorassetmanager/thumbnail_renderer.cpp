// thumbnailrenderer.cpp

#include <GL/glew.h>
#include "thumbnail_renderer.h"
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "core/engine/3d/staticmesh/staticmesh.h"
#include "core/engine/3d/staticmesh/staticmeshasset.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/engine/texture/textureasset.h"
#include "core/graphics/core/material/MMaterialAsset.h"
#include "core/graphics/core/material/material.h"
#include "core/graphics/core/render-pipeline/buffers/buffer_names.h"
#include "core/graphics/core/render-pipeline/buffers/headless/headless_render_buffer.h"
#include "core/graphics/core/render-pipeline/render_item.h"
#include "core/graphics/core/render-pipeline/stages/lighting/lighting_stage.h"
#include "core/graphics/core/render-pipeline/stages/opaque/opaque_stage.h"
#include "core/utils/logger.h"

// -- Built-in Lambert shader (mesh thumbnails only) ----------------------------

static const char* k_thumbVert = R"GLSL(
#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 vNormal;
out vec3 vFragPos;

void main()
{
    vec4 worldPos  = model * vec4(aPos, 1.0);
    vFragPos       = worldPos.xyz;
    vNormal        = mat3(transpose(inverse(model))) * aNormal;
    gl_Position    = projection * view * worldPos;
}
)GLSL";

static const char* k_thumbFrag = R"GLSL(
#version 460 core
in  vec3 vNormal;
in  vec3 vFragPos;
out vec4 FragColor;

uniform vec3      baseColor;
uniform sampler2D albedoTex;
uniform int       useAlbedoTex;

void main()
{
    vec3 N = normalize(vNormal);

    vec3 color = baseColor;
    if (useAlbedoTex == 1)
    {
        vec2 uv = vec2(
            atan(N.z, N.x) / (2.0 * 3.14159265) + 0.5,
            asin(clamp(N.y, -1.0, 1.0)) / 3.14159265 + 0.5
        );
        color *= texture(albedoTex, uv).rgb;
    }

    vec3 key  = normalize(vec3(-1.0, 2.0, 2.0));
    float kd  = max(dot(N, key), 0.0) * 0.75;

    vec3 fill = normalize(vec3(2.0, 0.5, 1.0));
    float fd  = max(dot(N, fill), 0.0) * 0.25;

    vec3 rim  = normalize(vec3(0.0, -1.0, -2.0));
    float rd  = pow(max(dot(N, rim), 0.0), 4.0) * 0.3;

    vec3 col  = color * (0.18 + kd + fd) + vec3(0.3) * rd;
    FragColor = vec4(col, 1.0);
}
)GLSL";

// -- Composite shader (BUFFER_OPAQUE x BUFFER_LIGHTS) --------------------------

static const char* k_compositeVert = R"GLSL(
#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;

out vec2 vUV;

void main()
{
    vUV         = aUV;
    gl_Position = vec4(aPos, 1.0);
}
)GLSL";

static const char* k_compositeFrag = R"GLSL(
#version 460 core
in  vec2 vUV;
out vec4 FragColor;

uniform sampler2D albedoTex;
uniform sampler2D lightsTex;

void main()
{
    vec4 opaque = texture(albedoTex, vUV);
    vec4 lights = texture(lightsTex, vUV);

    // alpha > 0 means the pixel was rendered by the lighting stage (lit).
    // alpha == 0 means unlit -- pass through the opaque value as-is.
    vec3 result = mix(opaque.rgb, opaque.rgb * lights.rgb, lights.a);
    FragColor   = vec4(result, 1.0);
}
)GLSL";

// -----------------------------------------------------------------------------

MThumbnailRenderer::~MThumbnailRenderer()
{
    destroyGL();
}

void MThumbnailRenderer::init()
{
    if (initialised) return;

    fbo.makeBuffer("thumbnail_fbo");
    if (!fbo.resize(THUMBNAIL_SIZE, THUMBNAIL_SIZE))
    {
        MERROR("MThumbnailRenderer::init -- failed to create thumbnail FBO");
        return;
    }

    buildThumbnailShader();
    buildSphereGeometry();
    buildCompositeResources();
    initThumbnailPipeline();

    initialised = thumbnailShader != 0;
    if (initialised)
        MLOG("MThumbnailRenderer:: Initialised");
}

// -- Thumbnail pipeline setup --------------------------------------------------

void MThumbnailRenderer::initThumbnailPipeline()
{
    thumbnailPipeline.initManual();
    thumbnailPipeline.setManualItemMode(true);

    // Opaque stage renders albedo into BUFFER_OPAQUE.
    // Lighting stage renders light contribution into BUFFER_LIGHTS.
    // We skip depth/shadow/skybox/composite -- composite is done manually.
    thumbnailPipeline.addStage<MOpaqueStage>();
    thumbnailPipeline.addStage<MLightingStage>();

    headlessRenderBuffer = new SHeadlessRenderBuffer(THUMBNAIL_SIZE, THUMBNAIL_SIZE);
    thumbnailPipeline.setRenderBuffer(headlessRenderBuffer);

    // Fixed light rig -- warm key from upper-left, soft white ambient.
    // lightDirection is the direction the light TRAVELS (entity forward vector).
    // The lighting shader negates it: dot(N, -lightDirection).
    // Source at upper-left means light travels toward lower-right.
    thumbnailPipeline.setLightOverride(
        glm::vec3(1.0f, 1.0f, 1.0f), 0.5f,                              // ambient
        glm::normalize(glm::vec3(0.5f, -1.5f, -1.0f)),                   // directional: travels from upper-left toward sphere
        glm::vec3(1.0f, 0.98f, 0.95f), 1.2f                              // directional colour + intensity
    );
}

// -- Request queue -------------------------------------------------------------

void MThumbnailRenderer::requestThumbnail(MAsset* asset, MThumbnailCache& cache)
{
    if (!asset) return;
    if (cache.has(asset->getAssetId())) return;

    EThumbnailAssetType type = EThumbnailAssetType::Unknown;
    if (dynamic_cast<MStaticMeshAsset*>(asset))
        type = EThumbnailAssetType::StaticMesh;
    else if (dynamic_cast<MMaterialAsset*>(asset))
        type = EThumbnailAssetType::Material;
    else
        return;

    auto copy = requestQueue;
    while (!copy.empty())
    {
        if (copy.front().assetId == asset->getAssetId()) return;
        copy.pop();
    }

    SAssetThumbnailRequest req;
    req.assetId = asset->getAssetId();
    req.asset   = asset;
    req.type    = type;
    requestQueue.push(req);
}

void MThumbnailRenderer::clearQueue()
{
    while (!requestQueue.empty())
        requestQueue.pop();
}

// -- Tick -- one render per frame ----------------------------------------------

bool MThumbnailRenderer::tick(MThumbnailCache& cache)
{
    if (!initialised || requestQueue.empty()) return false;

    SAssetThumbnailRequest req = requestQueue.front();
    requestQueue.pop();

    if (!req.asset) return false;
    if (cache.has(req.assetId)) return false;

    sf::Texture* thumb = nullptr;

    switch (req.type)
    {
        case EThumbnailAssetType::StaticMesh:
            thumb = renderMeshThumbnail(dynamic_cast<MStaticMeshAsset*>(req.asset));
            break;
        case EThumbnailAssetType::Material:
            thumb = renderMaterialThumbnail(dynamic_cast<MMaterialAsset*>(req.asset));
            break;
        default:
            break;
    }

    if (thumb)
        cache.store(req.assetId, thumb);

    return thumb != nullptr;
}

// -- Mesh thumbnail ------------------------------------------------------------

sf::Texture* MThumbnailRenderer::renderMeshThumbnail(MStaticMeshAsset* asset)
{
    if (!asset) return nullptr;
    auto meshes = asset->getMeshes();
    if (meshes.empty()) return nullptr;

    glm::vec3 aabbMin( 1e9f);
    glm::vec3 aabbMax(-1e9f);

    for (auto* m : meshes)
    {
        if (!m) continue;
        for (const auto& v : m->getVertices())
        {
            aabbMin = glm::min(aabbMin, v.Position);
            aabbMax = glm::max(aabbMax, v.Position);
        }
    }

    glm::vec3 center(0.f);
    float     radius = 1.0f;
    if (aabbMin.x < 1e8f)
    {
        center = (aabbMin + aabbMax) * 0.5f;
        radius = glm::length(aabbMax - center);
        radius = glm::max(radius, 0.001f);
    }

    std::vector<SMeshDraw> draws;
    for (auto* m : meshes)
    {
        if (!m) continue;
        SMeshDraw d;
        d.vao   = m->getVAO();
        d.ebo   = m->getEBO();
        d.count = m->getIndexCount();
        if (d.vao != 0) draws.push_back(d);
    }

    if (draws.empty()) return nullptr;

    SThumbnailCamera cam = computeThumbnailCamera(center, radius);

    fbo.bindAsActive();
    glViewport(0, 0, THUMBNAIL_SIZE, THUMBNAIL_SIZE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glClearColor(0.12f, 0.12f, 0.12f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::vec3 meshColor(0.72f, 0.72f, 0.75f);
    drawMeshes(draws, cam, meshColor);

    auto* tex = readbackToTexture();
    fbo.unbind();
    return tex;
}

// -- Material thumbnail (pipeline-driven) -------------------------------------

sf::Texture* MThumbnailRenderer::renderMaterialThumbnail(MMaterialAsset* asset)
{
    if (!asset || sphereVAO == 0) return nullptr;

    // getMaterial() returns the shared 'original' -- do NOT delete it.
    MMaterial* mat = asset->getMaterial();
    if (!mat || !mat->isValid())
    {
        // No usable shader -- fall back to the Lambert with a default colour.
        SThumbnailCamera cam = computeThumbnailCamera(glm::vec3(0.f), 1.0f);

        fbo.bindAsActive();
        glViewport(0, 0, THUMBNAIL_SIZE, THUMBNAIL_SIZE);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glClearColor(0.12f, 0.12f, 0.12f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::vec3 fallbackColor(0.85f, 0.45f, 0.15f);
        SMeshDraw sphere{ sphereVAO, sphereEBO, sphereIndexCount };
        drawMeshes({ sphere }, cam, fallbackColor);

        auto* tex = readbackToTexture();
        fbo.unbind();
        return tex;
    }

    // -- Build a render item for the unit sphere with this material -----------
    SRenderItem item;
    item.vao         = sphereVAO;
    item.ebo         = sphereEBO;
    item.indexCount   = sphereIndexCount;
    item.material    = mat;
    item.transform   = glm::mat4(1.f);
    item.castsShadow = false;

    // -- Set the thumbnail camera ---------------------------------------------
    SThumbnailCamera cam = computeThumbnailCamera(glm::vec3(0.f), 1.0f);
    thumbnailPipeline.setCameraOverride(cam.view, cam.proj);

    // -- Clear BUFFER_OPAQUE manually (no MClearStage in thumbnail pipeline) --
    auto* opaqueBuffer = thumbnailPipeline.getBufferRegistry()
                                           .getBuffer<SFrameBuffer>(MBufferNames::BUFFER_OPAQUE);
    if (!opaqueBuffer || opaqueBuffer->getFBOHandle() == 0)
    {
        MERROR("MThumbnailRenderer::renderMaterialThumbnail -- opaque buffer not ready");
        return nullptr;
    }

    opaqueBuffer->bindAsActive();
    glViewport(0, 0, THUMBNAIL_SIZE, THUMBNAIL_SIZE);
    glClearColor(0.12f, 0.12f, 0.12f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    opaqueBuffer->unbind();

    // -- Submit and drive the pipeline ----------------------------------------
    thumbnailPipeline.clearRenderItems();
    thumbnailPipeline.submitRenderItem(item);
    thumbnailPipeline.preRender();
    thumbnailPipeline.render();
    thumbnailPipeline.postRender();

    // -- Composite BUFFER_OPAQUE x BUFFER_LIGHTS into the thumbnail FBO -------
    compositeThumbnail();

    // -- Read back from the composited thumbnail FBO --------------------------
    fbo.bindAsActive();
    auto* tex = readbackToTexture();
    fbo.unbind();

    return tex;
}

// -- Composite helper ----------------------------------------------------------

void MThumbnailRenderer::compositeThumbnail()
{
    if (compositeShader == 0 || compositeQuadVAO == 0) return;

    auto* opaqueBuffer = thumbnailPipeline.getBufferRegistry()
                                           .getBuffer<SFrameBuffer>(MBufferNames::BUFFER_OPAQUE);
    auto* lightsBuffer = thumbnailPipeline.getBufferRegistry()
                                           .getBuffer<SFrameBuffer>(MBufferNames::BUFFER_LIGHTS);

    if (!opaqueBuffer || opaqueBuffer->getFBOHandle() == 0) return;

    // Render into the thumbnail's own FBO (not BUFFER_OPAQUE).
    fbo.bindAsActive();
    glViewport(0, 0, THUMBNAIL_SIZE, THUMBNAIL_SIZE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glClearColor(0.12f, 0.12f, 0.12f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(compositeShader);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, opaqueBuffer->getColorTextureHandle());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D,
                  (lightsBuffer && lightsBuffer->getFBOHandle() != 0)
                      ? lightsBuffer->getColorTextureHandle()
                      : opaqueBuffer->getColorTextureHandle());

    if (ucAlbedoTex >= 0) glUniform1i(ucAlbedoTex, 0);
    if (ucLightsTex >= 0) glUniform1i(ucLightsTex, 1);

    glBindVertexArray(compositeQuadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);

    glEnable(GL_DEPTH_TEST);
    fbo.unbind();
}

// -- Shared draw helper --------------------------------------------------------

void MThumbnailRenderer::drawMeshes(const std::vector<SMeshDraw>& meshes,
                                     const SThumbnailCamera& cam,
                                     const glm::vec3& baseColor)
{
    if (thumbnailShader == 0) return;

    glUseProgram(thumbnailShader);

    glm::mat4 model(1.f);
    glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(uView,  1, GL_FALSE, glm::value_ptr(cam.view));
    glUniformMatrix4fv(uProj,  1, GL_FALSE, glm::value_ptr(cam.proj));
    glUniform3fv(uBaseColor, 1, glm::value_ptr(baseColor));

    if (uUseAlbedoTex >= 0) glUniform1i(uUseAlbedoTex, 0);

    for (const auto& d : meshes)
    {
        if (d.vao == 0) continue;
        glBindVertexArray(d.vao);
        if (d.ebo != 0)
            glDrawElements(GL_TRIANGLES, d.count, GL_UNSIGNED_INT, nullptr);
        else
            glDrawArrays(GL_TRIANGLES, 0, d.count);
    }

    glBindVertexArray(0);
    glUseProgram(0);
}

// -- Pixel readback ------------------------------------------------------------

sf::Texture* MThumbnailRenderer::readbackToTexture()
{
    std::vector<uint8_t> pixels(THUMBNAIL_SIZE * THUMBNAIL_SIZE * 4);
    glReadPixels(0, 0, THUMBNAIL_SIZE, THUMBNAIL_SIZE,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    std::vector<uint8_t> flipped(pixels.size());
    const int rowBytes = THUMBNAIL_SIZE * 4;
    for (int y = 0; y < THUMBNAIL_SIZE; ++y)
    {
        const int srcRow = (THUMBNAIL_SIZE - 1 - y) * rowBytes;
        const int dstRow = y * rowBytes;
        std::memcpy(flipped.data() + dstRow, pixels.data() + srcRow, rowBytes);
    }

    auto* tex = new sf::Texture(sf::Vector2u(THUMBNAIL_SIZE, THUMBNAIL_SIZE));
    if (!tex)
    {
        delete tex;
        return nullptr;
    }
    tex->update(flipped.data());
    return tex;
}

// -- Camera helpers ------------------------------------------------------------

MThumbnailRenderer::SThumbnailCamera
MThumbnailRenderer::computeThumbnailCamera(const glm::vec3& center, float radius)
{
    const float halfFovRad = glm::radians(FOV_DEG * 0.5f);
    const float dist = (radius * MARGIN) / std::tan(halfFovRad);

    const glm::vec3 dir = glm::normalize(glm::vec3(1.2f, 0.8f, 1.6f));
    const glm::vec3 eye = center + dir * dist;

    const float nearZ = glm::max(dist - radius * MARGIN, 0.001f);
    const float farZ  = dist + radius * MARGIN;

    SThumbnailCamera cam;
    cam.view = glm::lookAt(eye, center, glm::vec3(0.f, 1.f, 0.f));
    cam.proj = glm::perspective(glm::radians(FOV_DEG), 1.f, nearZ, farZ);
    return cam;
}

// -- GL resource creation ------------------------------------------------------

void MThumbnailRenderer::buildThumbnailShader()
{
    auto compile = [](GLenum type, const char* src) -> GLuint {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[512]; glGetShaderInfoLog(s, 512, nullptr, log);
            MERROR(SString("MThumbnailRenderer: shader compile error: ") + log);
            glDeleteShader(s); return 0;
        }
        return s;
    };

    GLuint vs = compile(GL_VERTEX_SHADER,   k_thumbVert);
    GLuint fs = compile(GL_FRAGMENT_SHADER, k_thumbFrag);
    if (!vs || !fs) { glDeleteShader(vs); glDeleteShader(fs); return; }

    thumbnailShader = glCreateProgram();
    glAttachShader(thumbnailShader, vs);
    glAttachShader(thumbnailShader, fs);
    glLinkProgram(thumbnailShader);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok; glGetProgramiv(thumbnailShader, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512]; glGetProgramInfoLog(thumbnailShader, 512, nullptr, log);
        MERROR(SString("MThumbnailRenderer: shader link error: ") + log);
        glDeleteProgram(thumbnailShader);
        thumbnailShader = 0;
        return;
    }

    uModel        = glGetUniformLocation(thumbnailShader, "model");
    uView         = glGetUniformLocation(thumbnailShader, "view");
    uProj         = glGetUniformLocation(thumbnailShader, "projection");
    uBaseColor    = glGetUniformLocation(thumbnailShader, "baseColor");
    uAlbedoTex    = glGetUniformLocation(thumbnailShader, "albedoTex");
    uUseAlbedoTex = glGetUniformLocation(thumbnailShader, "useAlbedoTex");
}

void MThumbnailRenderer::buildCompositeResources()
{
    // -- Compile the composite multiply shader --------------------------------
    auto compile = [](GLenum type, const char* src) -> GLuint {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[512]; glGetShaderInfoLog(s, 512, nullptr, log);
            MERROR(SString("MThumbnailRenderer: composite shader compile error: ") + log);
            glDeleteShader(s); return 0;
        }
        return s;
    };

    GLuint vs = compile(GL_VERTEX_SHADER,   k_compositeVert);
    GLuint fs = compile(GL_FRAGMENT_SHADER, k_compositeFrag);
    if (!vs || !fs) { glDeleteShader(vs); glDeleteShader(fs); return; }

    compositeShader = glCreateProgram();
    glAttachShader(compositeShader, vs);
    glAttachShader(compositeShader, fs);
    glLinkProgram(compositeShader);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok; glGetProgramiv(compositeShader, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512]; glGetProgramInfoLog(compositeShader, 512, nullptr, log);
        MERROR(SString("MThumbnailRenderer: composite shader link error: ") + log);
        glDeleteProgram(compositeShader);
        compositeShader = 0;
        return;
    }

    ucAlbedoTex = glGetUniformLocation(compositeShader, "albedoTex");
    ucLightsTex = glGetUniformLocation(compositeShader, "lightsTex");

    // -- Build fullscreen quad (pos vec3 + uv vec2, stride 5 floats) ----------
    static const float quadVerts[] = {
        // pos              // uv
        -1.f, -1.f, 0.f,   0.f, 0.f,
         1.f, -1.f, 0.f,   1.f, 0.f,
         1.f,  1.f, 0.f,   1.f, 1.f,
        -1.f,  1.f, 0.f,   0.f, 1.f,
    };
    static const unsigned int quadIdx[] = { 0, 1, 2,  0, 2, 3 };

    glGenVertexArrays(1, &compositeQuadVAO);
    glGenBuffers(1, &compositeQuadVBO);
    glGenBuffers(1, &compositeQuadEBO);

    glBindVertexArray(compositeQuadVAO);

    glBindBuffer(GL_ARRAY_BUFFER, compositeQuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, compositeQuadEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIdx), quadIdx, GL_STATIC_DRAW);

    // location 0: position (vec3)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    // location 1: uv (vec2)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);
}

void MThumbnailRenderer::buildSphereGeometry()
{
    const int stacks = 24, slices = 24;
    std::vector<float>        verts;
    std::vector<unsigned int> indices;

    for (int i = 0; i <= stacks; ++i)
    {
        float phi = (float)i / stacks * glm::pi<float>();
        float v   = (float)i / stacks;
        for (int j = 0; j <= slices; ++j)
        {
            float theta = (float)j / slices * 2.f * glm::pi<float>();
            float u     = (float)j / slices;
            float x = std::sin(phi) * std::cos(theta);
            float y = std::cos(phi);
            float z = std::sin(phi) * std::sin(theta);
            // position
            verts.push_back(x); verts.push_back(y); verts.push_back(z);
            // normal (== position for unit sphere)
            verts.push_back(x); verts.push_back(y); verts.push_back(z);
            // uv
            verts.push_back(u); verts.push_back(v);
        }
    }

    for (int i = 0; i < stacks; ++i)
    {
        for (int j = 0; j < slices; ++j)
        {
            int a = i * (slices + 1) + j;
            int b = a + slices + 1;
            indices.push_back(a);   indices.push_back(b);   indices.push_back(a+1);
            indices.push_back(b);   indices.push_back(b+1); indices.push_back(a+1);
        }
    }
    sphereIndexCount = (int)indices.size();

    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);

    glBindVertexArray(sphereVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // position + normal + uv = 3 + 3 + 2 = 8 floats per vertex
    const int stride = 8 * sizeof(float);
    // location 0: position (vec3)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    // location 1: normal (vec3)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    // location 2: uv (vec2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));

    glBindVertexArray(0);
}

void MThumbnailRenderer::destroyGL()
{
    if (thumbnailShader) { glDeleteProgram(thumbnailShader);    thumbnailShader = 0; }
    if (sphereVAO)       { glDeleteVertexArrays(1, &sphereVAO); sphereVAO = 0; }
    if (sphereVBO)       { glDeleteBuffers(1, &sphereVBO);      sphereVBO = 0; }
    if (sphereEBO)       { glDeleteBuffers(1, &sphereEBO);      sphereEBO = 0; }

    if (compositeShader)  { glDeleteProgram(compositeShader);           compositeShader  = 0; }
    if (compositeQuadVAO) { glDeleteVertexArrays(1, &compositeQuadVAO); compositeQuadVAO = 0; }
    if (compositeQuadVBO) { glDeleteBuffers(1, &compositeQuadVBO);      compositeQuadVBO = 0; }
    if (compositeQuadEBO) { glDeleteBuffers(1, &compositeQuadEBO);      compositeQuadEBO = 0; }

    thumbnailPipeline.cleanup();
    delete headlessRenderBuffer;
    headlessRenderBuffer = nullptr;
}