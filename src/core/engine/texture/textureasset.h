//
// Created by ssj5v on 12-10-2024.
//

#ifndef TEXTUREASSET_H
#define TEXTUREASSET_H
#include "texture.h"
#include "core/engine/assetmanagement/asset/asset.h"

namespace pugi { class xml_node; }

// ── Texture import settings enums ─────────────────────────────────────────────

enum class ETextureFilterMin
{
    Nearest,
    Linear,
    NearestMipmapNearest,
    LinearMipmapNearest,
    NearestMipmapLinear,
    LinearMipmapLinear
};

enum class ETextureFilterMag
{
    Nearest,
    Linear
};

enum class ETextureWrap
{
    Repeat,
    MirroredRepeat,
    ClampToEdge,
    ClampToBorder
};

enum class ETextureCompression
{
    None,
    DXT1,   // BC1 - RGB, no alpha, highest compression (~6:1)
    DXT5    // BC3 - RGBA with alpha, good compression (~4:1)
};

// ── MTextureAsset ─────────────────────────────────────────────────────────────

SCRIPT_BIND_CLASS()
class MTextureAsset : public MAsset {
    DEFINE_OBJECT_SUBCLASS(MTextureAsset)
private:
    MTexture texture;

    // Import settings - persisted in the .meta file under <textureImport>
    ETextureFilterMin   filterMin   = ETextureFilterMin::Linear;
    ETextureFilterMag   filterMag   = ETextureFilterMag::Linear;
    ETextureWrap        wrapS       = ETextureWrap::Repeat;
    ETextureWrap        wrapT       = ETextureWrap::Repeat;
    int                 maxImportSize = 0; // 0 = no limit
    ETextureCompression compression = ETextureCompression::None;
    // Colour textures (albedo, UI, skybox faces) hold sRGB-encoded values and
    // are decoded to linear by the GPU on sample. Turn off for data textures
    // (normal maps, masks, roughness) so their values are read as-is.
    bool                srgb        = true;

public:
    MTextureAsset();
    explicit MTextureAsset(const SString& path);
    MTextureAsset(const SString& path, const pugi::xml_node& importSettings);

    SCRIPT_BIND_FUNC()
    virtual MTexture* getTexture();
    bool requestReload() override;

    // ── Import settings accessors ─────────────────────────────────────────
    ETextureFilterMin   getFilterMin()     const { return filterMin; }
    ETextureFilterMag   getFilterMag()     const { return filterMag; }
    ETextureWrap        getWrapS()         const { return wrapS; }
    ETextureWrap        getWrapT()         const { return wrapT; }
    int                 getMaxImportSize() const { return maxImportSize; }
    ETextureCompression getCompression()   const { return compression; }
    bool                isSrgb()           const { return srgb; }

    void setFilterMin(ETextureFilterMin f)     { filterMin = f; }
    void setFilterMag(ETextureFilterMag f)     { filterMag = f; }
    void setWrapS(ETextureWrap w)              { wrapS = w; }
    void setWrapT(ETextureWrap w)              { wrapT = w; }
    void setMaxImportSize(int s)               { maxImportSize = s; }
    void setCompression(ETextureCompression c) { compression = c; }
    void setSrgb(bool s)                       { srgb = s; }

    // ── Colour-space aware binding ────────────────────────────────────────
    // sRGB textures are stored as SRGB8_ALPHA8 with decoding switched OFF, so
    // ImGui, gizmos and anything else that draws them straight to the screen
    // sees the raw sRGB values, exactly as before.  The render pipeline binds
    // material textures through bindForRendering(), which switches decoding
    // ON so shaders read linear values.  endRenderingDecode() switches it back
    // off; the pipeline calls it once all of its stages have run.
    void bindForRendering(unsigned int textureUnit);
    static void endRenderingDecode();

    // ── Persistence ───────────────────────────────────────────────────────
    void loadImportSettings(const pugi::xml_node& node);
    bool saveImportSettings();

private:
    bool loadWithSettings();
    void applyGLParams();
    bool usesSrgbStorage() const;
};

#endif //TEXTUREASSET_H