//
// Created by ssj5v on 27-03-2025.
//

#ifndef CUBEMAPTEXTURE_H
#define CUBEMAPTEXTURE_H
#include "core/engine/texture/texture.h"


class MCubemapTexture : public MTexture {
private:
    unsigned int textureId;
public:
    //Override all properties of MTexture base class here
    bool loadFromMemory(const void* data, size_t size) override;
    bool loadFromPath(const SString& path) override;
    bool loadFromStream(sf::InputStream& stream) override;
    sf::Texture* getTexture() override {return nullptr;}
    unsigned int getTextureID() override {return textureId;};
    void bind(const unsigned int& location, const unsigned int& index) override;

    static MCubemapTexture* createCubeMap(std::vector<SString> files);
};



#endif //CUBEMAPTEXTURE_H
