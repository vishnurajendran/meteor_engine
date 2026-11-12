//
// Sprite entity - a textured quad in the world.
//

#pragma once
#ifndef SPRITE_ENTITY_H
#define SPRITE_ENTITY_H

#include "core/engine/assetmanagement/asset/asset_ref_handle.h"
#include "core/engine/assetmanagement/asset/field_asset_ref_types.h"
#include "core/engine/entities/spatial/spatial.h"
#include "core/engine/texture/textureasset.h"
#include "core/graphics/core/render-pipeline/interfaces/drawable_interface.h"
#include "core/utils/aabb.h"
#include "core/utils/field_engine_types.h"

class MMaterial;
class MShader;

// Draws a texture on a quad. The quad lies in the entity's local XY plane,
// facing +Z, centred on the entity, and is sized from the texture:
//     width  = texture width  / pixelsPerUnit
//     height = texture height / pixelsPerUnit
// (1 x 1 with no texture). The transform's scale applies on top.
//
// Each sprite renders with its own material instance, filled from the fields
// below - no material asset needed. Opaque sprites use sprite.mesl,
// transparent ones the standard transparent.mesl.
//   transparent = false: opaque + alpha cutout. Deferred-lit, fogged, can cast
//                        shadows. Hard edges; no sorting issues.
//   transparent = true:  alpha-blended in the transparent pass, forward-lit.
//                        Soft edges; sorted back to front; casts no shadow.
// Both sides render; a lit sprite is lit on whichever side faces the camera.
SCRIPT_BIND_CLASS()
class MSpriteEntity : public MSpatialEntity, public IMeteorDrawable
{
    DEFINE_SPATIAL_CLASS(MSpriteEntity)

    DECLARE_FIELD(texture,         TAssetRef<MTextureAsset>, {})
    DECLARE_FIELD(color,           SVector3, SVector3(1.0f))   // tint, linear RGB
    DECLARE_FIELD(opacity,         float,    1.0f)             // multiplies the texture's alpha
    DECLARE_FIELD(lit,             bool,     true)             // scene lighting on / off
    DECLARE_FIELD(transparent,     bool,     false)            // alpha-blend instead of cutout
    DECLARE_FIELD(cutoutThreshold, float,    0.5f)             // opaque mode: alpha below this is dropped
    DECLARE_FIELD(pixelsPerUnit,   float,    100.0f)
    DECLARE_FIELD(flipX,           bool,     false)
    DECLARE_FIELD(flipY,           bool,     false)
    DECLARE_FIELD(castsShadow,     bool,     false)            // opaque mode only

public:
    MSpriteEntity();
    ~MSpriteEntity() override;

    void submitRenderItem(IRenderItemCollector* collector) override;
    bool canDraw() override { return getEnabled(); }
    void onExit() override;

    SCRIPT_BIND_FUNC()
    void setTexture(TAssetHandle<MTextureAsset> asset) { texture = TAssetRef<MTextureAsset>(asset); }
    SCRIPT_BIND_FUNC()
    TAssetHandle<MTextureAsset> getTexture() const { return texture.get().getHandle(); }
    TAssetRef<MTextureAsset>    getTextureRef() const { return texture.get(); }

    SCRIPT_BIND_FUNC()
    SVector3 getColor() const          { return color.get(); }
    SCRIPT_BIND_FUNC()
    void     setColor(SVector3 c)      { color = c; }
    SCRIPT_BIND_FUNC()
    float    getOpacity() const        { return opacity.get(); }
    SCRIPT_BIND_FUNC()
    void     setOpacity(float v)       { opacity = glm::clamp(v, 0.0f, 1.0f); }
    SCRIPT_BIND_FUNC()
    bool     getLit() const            { return lit.get(); }
    SCRIPT_BIND_FUNC()
    void     setLit(bool v)            { lit = v; }
    SCRIPT_BIND_FUNC()
    bool     getTransparent() const    { return transparent.get(); }
    SCRIPT_BIND_FUNC()
    void     setTransparent(bool v)    { transparent = v; }
    SCRIPT_BIND_FUNC()
    float    getCutoutThreshold() const { return cutoutThreshold.get(); }
    SCRIPT_BIND_FUNC()
    void     setCutoutThreshold(float v) { cutoutThreshold = glm::clamp(v, 0.0f, 1.0f); }
    SCRIPT_BIND_FUNC()
    float    getPixelsPerUnit() const  { return pixelsPerUnit.get(); }
    SCRIPT_BIND_FUNC()
    void     setPixelsPerUnit(float v) { pixelsPerUnit = glm::max(v, 0.001f); }
    SCRIPT_BIND_FUNC()
    bool     getFlipX() const          { return flipX.get(); }
    SCRIPT_BIND_FUNC()
    void     setFlipX(bool v)          { flipX = v; }
    SCRIPT_BIND_FUNC()
    bool     getFlipY() const          { return flipY.get(); }
    SCRIPT_BIND_FUNC()
    void     setFlipY(bool v)          { flipY = v; }
    SCRIPT_BIND_FUNC()
    bool     getCastsShadow() const    { return castsShadow.get(); }
    SCRIPT_BIND_FUNC()
    void     setCastsShadow(bool v)    { castsShadow = v; }

    // Quad size in local units, before the transform's scale.
    SCRIPT_BIND_FUNC()
    SVector2 getSize() const;
    SCRIPT_BIND_FUNC()
    AABB     getBounds() const { return bounds; }

private:
    bool     syncMaterial();          // creates / updates the material instance
    SMatrix4 getQuadMatrix() const;   // entity transform * quad size & flips

    MMaterial* material       = nullptr;   // owned
    MShader*   materialShader = nullptr;   // owned by the shader asset
    AABB       bounds         = { {0,0,0}, {0,0,0} };
};

#endif // SPRITE_ENTITY_H
