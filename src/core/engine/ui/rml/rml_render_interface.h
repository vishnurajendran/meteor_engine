//
// rml_render_interface.h
//
// RmlUi's shipped OpenGL 3 renderer, with texture loading through the asset
// manager:
//
//   - LoadTexture resolves the source as an asset ("assets/ui/logo.png" or
//     "guid:<id>") and hands RmlUi the MTextureAsset's own GL texture. The
//     asset keeps owning it: ReleaseTexture never deletes an asset's texture,
//     only textures RmlUi made itself (font atlases, TGA files, filters).
//   - Textures used by the UI should have Type = UI in their import settings
//     (premultiplied alpha, which is what this renderer blends with). Others
//     still draw, with a warning once per texture.
//   - When any texture asset is reloaded, the UI subsystem calls
//     Rml::ReleaseTextures() so RmlUi asks for it again.
//

#ifndef RML_RENDER_INTERFACE_H
#define RML_RENDER_INTERFACE_H

#include <unordered_set>

#include "RmlUi_Renderer_GL3.h"

class MRmlRenderInterface : public RenderInterface_GL3
{
public:
    MRmlRenderInterface() = default;
    ~MRmlRenderInterface() override = default;

    Rml::TextureHandle LoadTexture(Rml::Vector2i& textureDimensions, const Rml::String& source) override;
    Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i sourceDimensions) override;
    void               ReleaseTexture(Rml::TextureHandle texture) override;

    // Forget which GL ids belong to assets; call after Rml::ReleaseTextures().
    void clearAssetTextures() { assetTextures.clear(); }

private:
    std::unordered_set<Rml::TextureHandle> ownedTextures;   // RmlUi's own - deleted on release
    std::unordered_set<Rml::TextureHandle> assetTextures;   // an MTextureAsset's - never deleted here
    std::unordered_set<std::string>        warnedNonUiTextures;
};

#endif // RML_RENDER_INTERFACE_H
