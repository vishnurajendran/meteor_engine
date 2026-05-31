//
// Created by ssj5v on 29-03-2025.
//

#include "cubemapasset.h"

#include <sstream>
#include "GL/glew.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/texture/textureasset.h"
#include "core/utils/fileio.h"
#include "core/utils/logger.h"
#include "cubemaptexture.h"
#include "pugixml.hpp"

#include "core/engine/subsystem/subsystem_registry.h"

const char* const MCubemapAsset::FACE_LABELS[FACE_COUNT] = {
    "right", "left", "top", "bottom", "back", "front"
};

// Only parses the cubemap XML and stores face paths.
// The actual GPU cubemap is built in deferredAssetLoad() after all
// independent assets (the face MTextureAssets) are available.

MCubemapAsset::MCubemapAsset(const SString& path)
{
    this->path = path;
    valid = false;
    facePaths.resize(FACE_COUNT);

    SString data;
    if (!FileIO::readFile(path, data))
    {
        MERROR(STR("MCubemapAsset: could not load file ") + path);
        return;
    }

    pugi::xml_document doc;
    doc.load_string(data.c_str());
    pugi::xml_node root = doc.child("cubemap");
    name = root.attribute("name").as_string();

    for (int i = 0; i < FACE_COUNT; ++i)
        facePaths[i] = root.child(FACE_LABELS[i]).attribute("src").as_string();

    // Mark valid so the importer keeps the asset.
    // The GPU cubemap doesn't exist yet — it's built in deferredAssetLoad().
    valid = true;
}

void MCubemapAsset::deferredAssetLoad(bool forced)
{
    // Skip if already built, unless a forced rebuild was requested
    if (texture && !forced) return;

    if (!buildCubemap())
    {
        MWARN("MCubemapAsset::deferredAssetLoad — failed to build cubemap: " + path);
        // valid stays true so the asset remains in the map for later retry
        // (e.g. via requestReload after the missing face is added).
    }
}

bool MCubemapAsset::buildCubemap()
{
    auto* assetManager = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>();
    std::vector<TAssetHandle<MTextureAsset>> faceAssets;

    for (int i = 0; i < FACE_COUNT; ++i)
    {
        if (facePaths[i].empty())
        {
            MERROR(STR("MCubemapAsset: empty face path for '") + FACE_LABELS[i] + "'");
            return false;
        }

        const auto faceAsset = assetManager->getAsset<MTextureAsset>(facePaths[i]);
        if (!faceAsset)
        {
            MERROR(STR("MCubemapAsset: failed to load face '") + FACE_LABELS[i]
                   + "' at path: " + facePaths[i]);
            return false;
        }
        faceAssets.push_back(faceAsset);
    }

    texture = MCubemapTexture::createCubeMap(faceAssets);
    return texture != nullptr;
}

MTexture* MCubemapAsset::getTexture() { return texture; }

SString MCubemapAsset::getFacePath(int index) const
{
    if (index < 0 || index >= FACE_COUNT) return "";
    return facePaths[index];
}

void MCubemapAsset::setFacePath(int index, const SString& facePath)
{
    if (index < 0 || index >= FACE_COUNT) return;
    facePaths[index] = facePath;
}

bool MCubemapAsset::save()
{
    pugi::xml_document doc;
    auto root = doc.append_child("cubemap");
    root.append_attribute("name").set_value(name.c_str());

    for (int i = 0; i < FACE_COUNT; ++i)
        root.append_child(FACE_LABELS[i]).append_attribute("src").set_value(facePaths[i].c_str());

    std::ostringstream oss;
    doc.save(oss);
    SString str = oss.str();
    if (!FileIO::writeFile(path, str))
    {
        MERROR("MCubemapAsset::save — failed to write " + path);
        return false;
    }
    return true;
}

bool MCubemapAsset::requestReload()
{
    // Clean up old GL cubemap texture to avoid leaking.
    // NOTE: MCubemapTexture has no destructor that calls glDeleteTextures,
    //       so we do it manually here.  This is a pre-existing gap — the
    //       class itself should own the cleanup, but fixing that is out of
    //       scope for this change.
    if (texture)
    {
        unsigned int texId = texture->getTextureID();
        if (texId != 0)
            glDeleteTextures(1, &texId);
        delete texture;
        texture = nullptr;
    }

    valid = buildCubemap();
    return valid;
}
