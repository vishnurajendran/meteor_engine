// thumbnailrenderer.h
// Suggested location: editor/editorassetmanager/thumbnails/
//
// ── What this does ────────────────────────────────────────────────────────────
// Renders asset previews into a shared 128×128 SFrameBuffer, reads the pixels
// back with glReadPixels, and stores the result as an sf::Texture in
// MThumbnailCache.  One request is processed per tick() call so the editor
// frame time stays predictable.
//
// ── Material ──────────────────────────────────────────────────────────────────
// Renders a UV sphere with the material's colour properties applied to the
// built-in 3-point Lambert shader. No .mesl dependency needed.
//
// ── MStaticMesh requirements ─────────────────────────────────────────────────
// MThumbnailRenderer calls these methods on MStaticMesh.  Add them if missing:
//
//   unsigned int getVAO()        const;   // GL vertex array object
//   unsigned int getEBO()        const;   // GL element buffer (0 if none)
//   int          getIndexCount() const;   // number of indices
//   int          getVertexCount()const;   // number of vertices (if no EBO)
//   AABB         getBounds()     const;   // world-space bounding box
//
// The AABB is used to fit the camera to the mesh.  If getBounds() isn't
// available yet, the renderer falls back to a unit-sphere camera distance.
// ─────────────────────────────────────────────────────────────────────────────

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

    // Call once after the GL context is active (e.g. at the end of
    // MEditorAssetManager::refresh()).
    void init();

    // Queue a thumbnail generation request.
    // If the cache already has a valid entry, the request is silently dropped.
    void requestThumbnail(MAsset* asset, MThumbnailCache& cache);

    // Process one queued request.  Call from the editor's run() loop, after
    // the main pipeline has finished its postRender() (GL state is clean).
    // Returns true if a thumbnail was generated this tick.
    bool tick(MThumbnailCache& cache);

    // Drop all queued requests (e.g. before a full refresh()).
    void clearQueue();

    bool isInitialised() const { return initialised; }

    // Disk cache — called by MEditorAssetManager on hot reload to invalidate
    // both the in-memory thumbnail and its persisted PNG on disk.
    static void deleteFromDisk(const SString& assetId);

private:
    void buildThumbnailShader();
    void buildSphereGeometry();     // UV sphere for shader/material previews
    void destroyGL();

    // Returns a new sf::Texture* (caller stores it in the cache), or nullptr.
    sf::Texture* renderMeshThumbnail (MStaticMeshAsset* asset);
    sf::Texture* renderMaterialThumbnail(MMaterialAsset* asset);

    // Disk persistence helpers.
    // saveToDisk()     — called after every successful render.
    // loadFromDisk()   — called in requestThumbnail() before queuing a render;
    //                    if a PNG is found the render is skipped entirely.
    // ensureCacheDir() — creates .engine_data/thumbnail_cache/ if absent.
    static void        saveToDisk   (const SString& assetId, sf::Texture* tex);
    static sf::Texture* loadFromDisk(const SString& assetId);
    static void        ensureCacheDir();

    // Shared FBO render → pixel readback → sf::Texture.
    sf::Texture* readbackToTexture();

    // Camera helpers.
    //
    // computeThumbnailCamera() calculates the view and projection matrices
    // that guarantee the bounding sphere (center, radius) is fully enclosed
    // inside the camera frustum with no clipping:
    //
    //   camera distance  d = radius / tan(HALF_FOV)   * MARGIN
    //   near clip plane    = d - radius               * MARGIN  (clamped > 0)
    //   far  clip plane    = d + radius               * MARGIN
    //
    // HALF_FOV is derived from FOV_DEG so the formula and the projection matrix
    // always agree — changing FOV_DEG is the only knob needed.
    static constexpr float FOV_DEG  = 35.0f;
    static constexpr float MARGIN   = 1.15f;   // 15% headroom around the sphere

    struct SThumbnailCamera { glm::mat4 view; glm::mat4 proj; };
    static SThumbnailCamera computeThumbnailCamera(const glm::vec3& center, float radius);

    // Draw a list of VAO/EBO/count triples using the thumbnail shader.
    struct SMeshDraw { unsigned int vao; unsigned int ebo; int count; };
    void drawMeshes(const std::vector<SMeshDraw>& meshes,
                    const SThumbnailCamera& cam,
                    const glm::vec3& baseColor);

private:
    bool              initialised     = false;
    SFrameBuffer      fbo;            // 128×128 colour + depth
    unsigned int      thumbnailShader = 0;  // GL program handle

    // UV sphere for material/shader previews
    unsigned int sphereVAO = 0;
    unsigned int sphereVBO = 0;
    unsigned int sphereEBO = 0;
    int          sphereIndexCount = 0;

    std::queue<SAssetThumbnailRequest> requestQueue;

    // Uniform locations cached after shader link
    int uModel     = -1;
    int uView      = -1;
    int uProj      = -1;
    int uBaseColor = -1;
};

#endif // THUMBNAIL_RENDERER_H