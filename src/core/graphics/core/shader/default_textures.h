//
// Default textures - fallbacks for texture slots with nothing assigned.
//

#pragma once
#ifndef DEFAULT_TEXTURES_H
#define DEFAULT_TEXTURES_H

// A material texture property with no texture assigned samples these
// instead of whatever happens to be left bound on its texture unit.
// White makes "texture × colour" reduce to just the colour, so an empty
// albedo / emission / mask slot behaves as if there were no texture.
namespace MDefaultTextures
{
    // 1x1 opaque white (RGBA8). Created on first use; needs a GL context.
    unsigned int white();

    // Binds white() to GL_TEXTURE0 + unit (leaves that unit active).
    void bindWhite(unsigned int unit);
}

#endif // DEFAULT_TEXTURES_H
