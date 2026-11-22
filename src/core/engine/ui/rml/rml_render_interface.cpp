//
// rml_render_interface.cpp
//

#include "rml_render_interface.h"

#include "core/engine/assetmanagement/assetmanager/asset_manager_subsystem.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/engine/texture/textureasset.h"
#include "core/utils/fileio.h"
#include "core/utils/logger.h"
#include "rml_file_interface.h"
#include "rml_gl3_output.h"

// -- Output framebuffer (see rml_gl3_output.h) -------------------------------------------

namespace
{
    unsigned int g_outputFramebuffer = 0;
}

unsigned int RmlGL3_GetOutputFramebuffer()
{
    return g_outputFramebuffer;
}

void RmlGL3_SetOutputFramebuffer(unsigned int framebuffer)
{
    g_outputFramebuffer = framebuffer;
}

// -- Textures -----------------------------------------------------------------------------

Rml::TextureHandle MRmlRenderInterface::LoadTexture(Rml::Vector2i& textureDimensions, const Rml::String& source)
{
    const SString path = MRmlFileInterface::resolvePath(source);
    auto* manager = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>();

    if (manager && !path.empty())
    {
        if (MTextureAsset* asset = manager->getAsset<MTextureAsset>(path).get())
        {
            MTexture* texture = asset->getTexture();
            const unsigned int glId = texture ? texture->getTextureID() : 0;
            if (glId != 0)
            {
                if (!asset->isUITexture() && warnedNonUiTextures.insert(path.str()).second)
                    MWARN(SString::format("UI: texture '{0}' is used by the UI but its Type is not UI - set Type to UI "
                                          "in its import settings so transparent edges blend correctly", path));

                const SVector2 size = texture->getSize();
                textureDimensions = Rml::Vector2i(static_cast<int>(size.x), static_cast<int>(size.y));
                const auto handle = static_cast<Rml::TextureHandle>(glId);
                assetTextures.insert(handle);
                return handle;
            }
        }
    }

    // RmlUi's own loader reads TGA files (through the file interface).
    if (FileIO::getFileExtension(path).toLower() == "tga")
    {
        const Rml::TextureHandle handle = RenderInterface_GL3::LoadTexture(textureDimensions, source);
        if (handle)
            ownedTextures.insert(handle);
        return handle;
    }

    MERROR(SString::format("UI: '{0}' is not a texture asset", SString(source)));
    return 0;
}

Rml::TextureHandle MRmlRenderInterface::GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i sourceDimensions)
{
    const Rml::TextureHandle handle = RenderInterface_GL3::GenerateTexture(source, sourceDimensions);
    if (handle)
        ownedTextures.insert(handle);
    return handle;
}

void MRmlRenderInterface::ReleaseTexture(Rml::TextureHandle texture)
{
    if (ownedTextures.erase(texture) > 0)
    {
        RenderInterface_GL3::ReleaseTexture(texture);
        return;
    }
    // An asset's texture: the asset owns it.
    assetTextures.erase(texture);
}
