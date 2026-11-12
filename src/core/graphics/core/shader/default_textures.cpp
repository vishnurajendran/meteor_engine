//
// Default textures - fallbacks for texture slots with nothing assigned.
//

#include <GL/glew.h>
#include "default_textures.h"

namespace MDefaultTextures
{

unsigned int white()
{
    // Lives for the whole run - the GL context is torn down with the process.
    static GLuint texture = 0;
    if (texture == 0)
    {
        const unsigned char pixel[4] = { 255, 255, 255, 255 };
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    return texture;
}

void bindWhite(unsigned int unit)
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, white());
}

} // namespace MDefaultTextures
