//
// Texture buffer - a single colour texture, no FBO of its own.
//

#include <GL/glew.h>
#include "texture_buffer.h"
#include "core/utils/logger.h"

STextureBuffer::~STextureBuffer()
{
    destroyGL();
}

bool STextureBuffer::makeBuffer(const SString& bufferName)
{
    name = bufferName;
    return true; // allocation deferred to resize()
}

bool STextureBuffer::resize(int width, int height)
{
    if (width <= 0 || height <= 0)
    {
        MERROR("STextureBuffer::resize - invalid dimensions for '" + name + "'");
        return false;
    }

    destroyGL();
    resolution = { static_cast<float>(width), static_cast<float>(height) };

    // Upload type only matters for the (null) initial data, but must be
    // compatible with the internal format.
    GLenum dataFormat = GL_RGBA;
    GLenum dataType   = GL_UNSIGNED_BYTE;
    switch (internalFormat)
    {
        case GL_RGBA16F: case GL_RGBA32F:                 dataType = GL_FLOAT; break;
        case GL_RG16F:   case GL_RG32F:  dataFormat = GL_RG;  dataType = GL_FLOAT; break;
        case GL_R16F:    case GL_R32F:   dataFormat = GL_RED; dataType = GL_FLOAT; break;
        case GL_RG8:                     dataFormat = GL_RG;  break;
        case GL_R8:                      dataFormat = GL_RED; break;
        default: break;
    }

    // Nearest filtering: these are per-pixel data (normals, material values)
    // that must stay paired with the depth texel at the same coordinate.
    glGenTextures(1, &textureHandle);
    glBindTexture(GL_TEXTURE_2D, textureHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(internalFormat), width, height, 0,
                 dataFormat, dataType, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

void STextureBuffer::destroyGL()
{
    if (textureHandle) { glDeleteTextures(1, &textureHandle); textureHandle = 0; }
}
