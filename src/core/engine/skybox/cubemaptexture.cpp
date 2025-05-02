//
// Created by ssj5v on 27-03-2025.
//
#include "GL/glew.h"
#include "cubemaptexture.h"

#include "core/utils/fileio.h"
#include "SFML/System/InputStream.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
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
createCubeMap(std::vector<SString> files)
{
    MCubemapTexture* cubemap = new MCubemapTexture();
    int width, height, channels;
    unsigned int textureId=0;
    unsigned char *data = nullptr;

    if (files.size() == 0)
    {
        MERROR(STR("CubeMap Textures not defined"));
        return nullptr;
    }

    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureId);
    for(unsigned int i = 0; i < files.size(); i++)
    {
        data = stbi_load(files[i].c_str(), &width, &height, &channels, 0);
        if (data == nullptr)
        {
            MERROR(STR("Problem loading CubeMapTexture file ") + files[i] + " issue: File not found or cannot be read");
            return nullptr;
        }

        auto colorFormat = channels == 4 ? GL_RGBA : GL_RGB;

        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            0, colorFormat, width, height, 0, colorFormat, GL_UNSIGNED_BYTE, data
        );
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    cubemap->textureId = textureId;
    return cubemap;
}


