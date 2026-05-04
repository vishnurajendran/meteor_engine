// thumbnailrenderer.cpp

#include <GL/glew.h>
#include "thumbnail_renderer.h"
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "core/engine/3d/staticmesh/staticmesh.h"
#include "core/engine/3d/staticmesh/staticmeshasset.h"
#include "core/graphics/core/material/MMaterialAsset.h"
#include "core/graphics/core/material/material.h"
#include "core/utils/logger.h"

// ── Built-in Lambert shader ───────────────────────────────────────────────────
// Three-point light rig: key (warm), fill (cool), rim (white).
// Normal is world-space from the geometry; no textures needed.
// ─────────────────────────────────────────────────────────────────────────────

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

uniform vec3 baseColor;

void main()
{
    vec3 N   = normalize(vNormal);

    // Key light — warm, upper-left
    vec3 key  = normalize(vec3(-1.0, 2.0, 2.0));
    float kd  = max(dot(N, key), 0.0) * 0.75;

    // Fill light — cool, right
    vec3 fill = normalize(vec3(2.0, 0.5, 1.0));
    float fd  = max(dot(N, fill), 0.0) * 0.25;

    // Rim light — back edge highlight
    vec3 rim  = normalize(vec3(0.0, -1.0, -2.0));
    float rd  = pow(max(dot(N, rim), 0.0), 4.0) * 0.3;

    vec3 col  = baseColor * (0.18 + kd + fd) + vec3(0.3) * rd;
    FragColor = vec4(col, 1.0);
}
)GLSL";

// ─────────────────────────────────────────────────────────────────────────────

MThumbnailRenderer::~MThumbnailRenderer()
{
    destroyGL();
}

void MThumbnailRenderer::init()
{
    if (initialised) return;

    // The FBO makeBuffer() defers allocation to resize(); call resize() directly.
    fbo.makeBuffer("thumbnail_fbo");
    if (!fbo.resize(THUMBNAIL_SIZE, THUMBNAIL_SIZE))
    {
        MERROR("MThumbnailRenderer::init — failed to create thumbnail FBO");
        return;
    }

    buildThumbnailShader();
    buildSphereGeometry();

    initialised = thumbnailShader != 0;
    if (initialised)
        MLOG("MThumbnailRenderer:: Initialised");
}

// ── Request queue ─────────────────────────────────────────────────────────────

void MThumbnailRenderer::requestThumbnail(MAsset* asset, MThumbnailCache& cache)
{
    if (!asset) return;
    if (cache.has(asset->getAssetId())) return; // already cached

    // Determine type
    EThumbnailAssetType type = EThumbnailAssetType::Unknown;
    if (dynamic_cast<MStaticMeshAsset*>(asset))
        type = EThumbnailAssetType::StaticMesh;
    else if (dynamic_cast<MMaterialAsset*>(asset))
        type = EThumbnailAssetType::Material;
    else
        return; // no thumbnail for this asset type (shaders use their file icon)

    // Avoid duplicate queue entries
    // (Simple O(n) scan — the queue is tiny, never more than a few hundred)
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

// ── Tick — one render per frame ───────────────────────────────────────────────

bool MThumbnailRenderer::tick(MThumbnailCache& cache)
{
    if (!initialised || requestQueue.empty()) return false;

    SAssetThumbnailRequest req = requestQueue.front();
    requestQueue.pop();

    // Asset may have been deleted since the request was queued.
    if (!req.asset) return false;

    // Skip if it was cached by a previous tick (can happen after hot reload
    // followed by a fresh request for the same asset).
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

// ── Mesh thumbnail ────────────────────────────────────────────────────────────

sf::Texture* MThumbnailRenderer::renderMeshThumbnail(MStaticMeshAsset* asset)
{
    if (!asset) return nullptr;
    auto meshes = asset->getMeshes();
    if (meshes.empty()) return nullptr;

    // ── Compute local-space AABB from raw vertex positions ────────────────────
    // Bounds are not stored on MStaticMesh (they depend on the transform
    // matrix in world use). For thumbnail purposes we compute them here
    // directly from the vertex data — no transform needed since we centre
    // the camera on whatever space the vertices happen to be in.
    //
    // Requires MStaticMesh to expose its vertices:
    //   const std::vector<SVertex>& getVertices() const { return vertices; }
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

    // Bounding sphere that fully encloses the AABB.
    // Half-diagonal guarantees all 8 corners are inside the sphere,
    // so if the sphere fits the frustum the entire mesh does too.
    glm::vec3 center(0.f);
    float     radius = 1.0f;    // fallback for meshes with no vertices
    if (aabbMin.x < 1e8f)
    {
        center = (aabbMin + aabbMax) * 0.5f;
        radius = glm::length(aabbMax - center);
        radius = glm::max(radius, 0.001f);
    }

    // ── Build draw list ───────────────────────────────────────────────────────
    // Requires MStaticMesh::getVAO(), getEBO(), getIndexCount().
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

    // ── Camera: frustum-fit to bounding sphere ────────────────────────────────
    SThumbnailCamera cam = computeThumbnailCamera(center, radius);

    // ── Render ────────────────────────────────────────────────────────────────
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

// ── Material thumbnail ──────────────────────────────────────────────────────────

sf::Texture* MThumbnailRenderer::renderMaterialThumbnail(MMaterialAsset* asset)
{
    if (!asset || sphereVAO == 0) return nullptr;

    // getInstance() returns a cloned MMaterial with all per-material property
    // overrides applied on top of the shader defaults. We own this clone.
    MMaterial* mat = asset->getMaterial();
    glm::vec3 baseColor(0.85f, 0.45f, 0.15f); // warm orange default

    if (mat)
    {
        for (auto& [key, val] : mat->getProperties())
        {
            if (val.getType() == SShaderPropertyType::Color)
            {
                auto c = val.getColor();
                baseColor = glm::vec3(c.r, c.g, c.b);
                break;
            }
            if (val.getType() == SShaderPropertyType::UniformVec4)
            {
                auto c = val.getVec4Val();
                baseColor = glm::vec3(c.r, c.g, c.b);
                break;
            }
            if (val.getType() == SShaderPropertyType::UniformVec3)
            {
                baseColor = val.getVec3Val();
                break;
            }
        }
        delete mat;
    }

    // Clamp so thumbnails are never pure-black or blown-out.
    baseColor = glm::clamp(baseColor, glm::vec3(0.05f), glm::vec3(1.0f));

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

    SMeshDraw sphere{ sphereVAO, sphereEBO, sphereIndexCount };
    drawMeshes({ sphere }, cam, baseColor);

    auto* tex = readbackToTexture();
    fbo.unbind();
    return tex;
}

// ── Shared draw helper ────────────────────────────────────────────────────────

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

// ── Pixel readback ────────────────────────────────────────────────────────────

sf::Texture* MThumbnailRenderer::readbackToTexture()
{
    // Read RGBA pixels from the currently-bound FBO's colour attachment.
    std::vector<uint8_t> pixels(THUMBNAIL_SIZE * THUMBNAIL_SIZE * 4);
    glReadPixels(0, 0, THUMBNAIL_SIZE, THUMBNAIL_SIZE,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    // OpenGL stores rows bottom-up; SFML expects top-down — flip vertically.
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

// ── Camera helpers ────────────────────────────────────────────────────────────
//
// Frustum-fit derivation
// ──────────────────────
// We want the bounding sphere (center, radius) to be completely inside the
// frustum with MARGIN headroom on every side.
//
// For a perspective camera with half-FOV angle A:
//   Any point on the sphere edge is at most `radius` from the center.
//   The frustum half-width at distance d is  d * tan(A).
//   For the sphere to fit:  d * tan(A) >= radius * MARGIN
//                        →  d >= radius * MARGIN / tan(A)
//
// The same distance d works for both horizontal and vertical because the
// thumbnail FBO is square (aspect = 1).
//
// Near plane: d - radius*MARGIN  (just in front of the sphere)
// Far  plane: d + radius*MARGIN  (just behind the sphere)
//
// Both are derived from d so a change to FOV_DEG or MARGIN automatically
// keeps the projection and distance in sync.

MThumbnailRenderer::SThumbnailCamera
MThumbnailRenderer::computeThumbnailCamera(const glm::vec3& center, float radius)
{
    const float halfFovRad = glm::radians(FOV_DEG * 0.5f);

    // Distance from eye to center that puts the bounding sphere edge exactly
    // at the frustum boundary, then scaled out by MARGIN.
    const float dist = (radius * MARGIN) / std::tan(halfFovRad);

    // Fixed 3/4 view direction — visually clean for any mesh orientation.
    const glm::vec3 dir = glm::normalize(glm::vec3(1.2f, 0.8f, 1.6f));
    const glm::vec3 eye = center + dir * dist;

    // Near/far derived from the same distance so there is no mismatch.
    const float nearZ = glm::max(dist - radius * MARGIN, 0.001f);
    const float farZ  = dist + radius * MARGIN;

    SThumbnailCamera cam;
    cam.view = glm::lookAt(eye, center, glm::vec3(0.f, 1.f, 0.f));
    cam.proj = glm::perspective(glm::radians(FOV_DEG), 1.f, nearZ, farZ);
    return cam;
}

// ── GL resource creation ──────────────────────────────────────────────────────

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

    uModel     = glGetUniformLocation(thumbnailShader, "model");
    uView      = glGetUniformLocation(thumbnailShader, "view");
    uProj      = glGetUniformLocation(thumbnailShader, "projection");
    uBaseColor = glGetUniformLocation(thumbnailShader, "baseColor");
}

void MThumbnailRenderer::buildSphereGeometry()
{
    // UV sphere — 32 stacks × 32 slices with position + normal at layout 0/1.
    const int stacks = 24, slices = 24;
    std::vector<float>        verts;
    std::vector<unsigned int> indices;

    for (int i = 0; i <= stacks; ++i)
    {
        float phi   = (float)i / stacks * glm::pi<float>();
        for (int j = 0; j <= slices; ++j)
        {
            float theta = (float)j / slices * 2.f * glm::pi<float>();
            float x = std::sin(phi) * std::cos(theta);
            float y = std::cos(phi);
            float z = std::sin(phi) * std::sin(theta);
            // position
            verts.push_back(x); verts.push_back(y); verts.push_back(z);
            // normal == position for unit sphere
            verts.push_back(x); verts.push_back(y); verts.push_back(z);
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

    const int stride = 6 * sizeof(float);
    // location 0: position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    // location 1: normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));

    glBindVertexArray(0);
}

void MThumbnailRenderer::destroyGL()
{
    if (thumbnailShader) { glDeleteProgram(thumbnailShader);    thumbnailShader = 0; }
    if (sphereVAO)       { glDeleteVertexArrays(1, &sphereVAO); sphereVAO = 0; }
    if (sphereVBO)       { glDeleteBuffers(1, &sphereVBO);      sphereVBO = 0; }
    if (sphereEBO)       { glDeleteBuffers(1, &sphereEBO);      sphereEBO = 0; }
    // SFrameBuffer cleans up its own GL resources in its destructor.
}