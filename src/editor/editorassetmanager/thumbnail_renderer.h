// thumbnailrenderer.h
//
// Renders asset previews into a shared 128×128 SFrameBuffer, reads the pixels
// back with glReadPixels, and stores the result as an sf::Texture in
// MThumbnailCache.  One request is processed per tick() call so the editor
// frame time stays predictable.
//
// ── Material ──────────────────────────────────────────────────────────────────
// Renders a UV sphere with the material's colour and texture applied via the
// built-in 3-point Lambert shader. Textures are mapped using spherical UVs
// derived from the surface normal — no vertex UV attribute needed.
//
// ── MStaticMesh requirements ─────────────────────────────────────────────────
// MThumbnailRenderer calls these methods on MStaticMesh:
//
//   unsigned int getVAO()        const;
//   unsigned int getEBO()        const;
//   int          getIndexCount() const;
//   int          getVertexCount()const;
//   AABB         getBounds()     const;

#pragma once
#ifndef THUMBNAIL_RENDERER_H
#define THUMBNAIL_RENDERER_H

#include <queue>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <filesystem>
#include "asset_thumbnail_request.h"
#include "core/graphics/core/render-pipeline/buffers/frame/frame_buffer.h"
#include "thumbnail_cache.h"

class MStaticMeshAsset;
class MMaterialAsset;

class MThumbnailRenderer
{
public:
    static constexpr int         THUMBNAIL_SIZE = 128;
    static constexpr const char* CACHE_DIR     = ".engine_data/thumbnail_cache/";

    MThumbnailRenderer()  = default;
    ~MThumbnailRenderer();

    void init();

    void requestThumbnail(MAsset* asset, MThumbnailCache& cache);
    bool tick(MThumbnailCache& cache);
    void clearQueue();

    bool isInitialised() const { return initialised; }

    static void deleteFromDisk(const SString& assetId);

private:
    void buildThumbnailShader();
    void buildSphereGeometry();
    void destroyGL();

    sf::Texture* renderMeshThumbnail    (MStaticMeshAsset* asset);
    sf::Texture* renderMaterialThumbnail(MMaterialAsset*   asset);

    static void         saveToDisk   (const SString& assetId, sf::Texture* tex);
    static sf::Texture* loadFromDisk (const SString& assetId);
    static void         ensureCacheDir();

    sf::Texture* readbackToTexture();

    static constexpr float FOV_DEG  = 35.0f;
    static constexpr float MARGIN   = 1.15f;

    struct SThumbnailCamera { glm::mat4 view; glm::mat4 proj; };
    static SThumbnailCamera computeThumbnailCamera(const glm::vec3& center, float radius);

    struct SMeshDraw { unsigned int vao; unsigned int ebo; int count; };
    void drawMeshes(const std::vector<SMeshDraw>& meshes,
                    const SThumbnailCamera& cam,
                    const glm::vec3& baseColor);

private:
    bool              initialised     = false;
    SFrameBuffer      fbo;
    unsigned int      thumbnailShader = 0;

    // UV sphere for material/shader previews
    unsigned int sphereVAO = 0;
    unsigned int sphereVBO = 0;
    unsigned int sphereEBO = 0;
    int          sphereIndexCount = 0;

    std::queue<SAssetThumbnailRequest> requestQueue;

    // Uniform locations cached after shader link
    int uModel        = -1;
    int uView         = -1;
    int uProj         = -1;
    int uBaseColor    = -1;
    int uAlbedoTex    = -1;
    int uUseAlbedoTex = -1;
};

#endif // THUMBNAIL_RENDERER_H