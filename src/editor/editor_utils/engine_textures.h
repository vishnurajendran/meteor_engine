//
// engine_textures.h
//
// Loads editor UI images that ship with the engine (meteor_assets/...).
//
// These live in the engine install (ENGINE_PATH), not in the project, so a
// plain sf::Texture::loadFromFile("meteor_assets/...") only works when the
// project is the engine folder. The sf::Texture(path) constructor also throws
// in SFML 3 when the file is missing, which crashed the editor on new projects.
//

#ifndef ENGINE_TEXTURES_H
#define ENGINE_TEXTURES_H

#include "SFML/Graphics/Texture.hpp"

// Loads ENGINE_PATH(relativePath) into `texture`. Never throws; logs a
// warning and leaves the texture empty on failure. Returns success.
bool loadEngineTexture(sf::Texture& texture, const char* relativePath);

// Same, returning the texture (empty on failure).
sf::Texture loadEngineTexture(const char* relativePath);

#endif // ENGINE_TEXTURES_H
