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

public:
    MTextureAsset();
    explicit MTextureAsset(const SString& path);
    MTextureAsset(const SString& path, const pugi::xml_node& importSettings);

    virtual MTexture* getTexture();
    bool requestReload() override;

    // ── Import settings accessors ─────────────────────────────────────────
    ETextureFilterMin   getFilterMin()     const { return filterMin; }
    ETextureFilterMag   getFilterMag()     const { return filterMag; }
    ETextureWrap        getWrapS()         const { return wrapS; }
    ETextureWrap        getWrapT()         const { return wrapT; }
    int                 getMaxImportSize() const { return maxImportSize; }
    ETextureCompression getCompression()   const { return compression; }

    void setFilterMin(ETextureFilterMin f)     { filterMin = f; }
    void setFilterMag(ETextureFilterMag f)     { filterMag = f; }
    void setWrapS(ETextureWrap w)              { wrapS = w; }
    void setWrapT(ETextureWrap w)              { wrapT = w; }
    void setMaxImportSize(int s)               { maxImportSize = s; }
    void setCompression(ETextureCompression c) { compression = c; }

    // ── Persistence ───────────────────────────────────────────────────────
    void loadImportSettings(const pugi::xml_node& node);
    bool saveImportSettings();

private:
    bool loadWithSettings();
    void applyGLParams();
};

#endif //TEXTUREASSET_H