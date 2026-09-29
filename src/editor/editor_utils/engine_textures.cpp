//
// engine_textures.cpp
//

#include "engine_textures.h"

#include "core/utils/logger.h"
#include "core/utils/meteor_paths.h"

bool loadEngineTexture(sf::Texture& texture, const char* relativePath)
{
    const SString fullPath = ENGINE_PATH(relativePath);
    if (!texture.loadFromFile(fullPath.str()))
    {
        MWARN(SString("EngineTextures:: missing engine image ") + fullPath);
        return false;
    }
    return true;
}

sf::Texture loadEngineTexture(const char* relativePath)
{
    sf::Texture texture;
    loadEngineTexture(texture, relativePath);
    return texture;
}
