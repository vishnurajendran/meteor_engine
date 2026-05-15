//
// Created by ssj5v on 27-03-2025.
//
#include "cubemaptexture.h"
#include "GL/glew.h"

#include "core/engine/assetmanagement/asset/asset_handle.h"
#include "core/engine/texture/textureasset.h"
#include "core/utils/logger.h"


bool MCubemapTexture::loadFromMemory(const void* data, size_t size)
{
    MERROR("SINGLE TEXTURE NOT SUPPORTED, USE CREATE METHOD INSTEAD!!");
    return false;
}

bool MCubemapTexture::loadFromPath(const SString& path)
{
    MERROR("SINGLE TEXTURE NOT SUPPORTED, USE CREATE METHOD INSTEAD!!");
    return false;
}

bool MCubemapTexture::loadFromStream(sf::InputStream& stream)
{
    MERROR("SINGLE TEXTURE NOT SUPPORTED, USE CREATE METHOD INSTEAD!!");
    return false;
}
void MCubemapTexture::bind(const unsigned int& location, const unsigned int& index)
{
    auto textureId = getTextureID();
    glActiveTexture(GL_TEXTURE0+index);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureId);
}

MCubemapTexture* MCubemapTexture::
createCubeMap(std::vector<TAssetHandle<MTextureAsset>> textureAssets)
{
    if (textureAssets.size() != 6)
    {
        MERROR(STR("CubeMap requires exactly 6 texture assets, got ") + std::to_string(textureAssets.size()));
        return nullptr;
    }

    // Validate all assets and their textures
    for (unsigned int i = 0; i < 6; i++)
    {
        if (!textureAssets[i] || !textureAssets[i]->getTexture() || !textureAssets[i]->getTexture()->getCoreTexture())
        {
            MERROR(STR("CubeMap face ") + std::to_string(i) + " has an invalid or null texture asset");
            return nullptr;
        }
    }

    // Use the first face to determine cubemap dimensions
    auto refSize = textureAssets[0]->getTexture()->getCoreTexture()->getSize();
    unsigned int width  = refSize.x;
    unsigned int height = refSize.y;

    // Verify all faces share the same dimensions
    for (unsigned int i = 1; i < 6; i++)
    {
        auto faceSize = textureAssets[i]->getTexture()->getCoreTexture()->getSize();
        if (faceSize.x != width || faceSize.y != height)
        {
            MERROR(STR("CubeMap face ") + std::to_string(i)
                   + " size (" + std::to_string(faceSize.x) + "x" + std::to_string(faceSize.y)
                   + ") does not match face 0 (" + std::to_string(width) + "x" + std::to_string(height) + ")");
            return nullptr;
        }
    }

    // Allocate cubemap with immutable storage (SFML uses RGBA8 internally)
    unsigned int cubemapId = 0;
    glGenTextures(1, &cubemapId);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapId);
    glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, GL_RGBA8, width, height);

    // Copy each face from its source 2D texture directly on the GPU
    for (unsigned int i = 0; i < 6; i++)
    {
        unsigned int srcId = textureAssets[i]->getTexture()->getTextureID();
        glCopyImageSubData(
            srcId,      GL_TEXTURE_2D,       0, 0, 0, 0,
            cubemapId,  GL_TEXTURE_CUBE_MAP, 0, 0, 0, i,
            width, height, 1
        );
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    MCubemapTexture* cubemap = new MCubemapTexture();
    cubemap->textureId = cubemapId;
    return cubemap;
}