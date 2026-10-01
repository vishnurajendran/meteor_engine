//
// Created by ssj5v on 29-03-2025.
//

#include "cubemapasset.h"

#include <sstream>
#include "GL/glew.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/texture/textureasset.h"
#include "core/engine/assetmanagement/source/asset_sources.h"
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
    facePaths.resize(FACE_COUNT);

    // Mark valid so the importer keeps the asset.
    // The GPU cubemap doesn't exist yet - it's built in deferredAssetLoad().
    valid = parseDefinition();
}

bool MCubemapAsset::parseDefinition()
{
    std::vector<uint8_t> bytes;
    if (!MAssetSources::getActive()->readBytes(path, bytes))
    {
        MERROR(STR("MCubemapAsset: could not load file ") + path);
        return false;
    }

    pugi::xml_document doc;
    if (doc.load_buffer(bytes.data(), bytes.size()).status != pugi::status_ok)
    {
        MERROR(STR("MCubemapAsset: could not parse file ") + path);
        return false;
    }

    pugi::xml_node root = doc.child("cubemap");
    name = root.attribute("name").as_string();

    for (int i = 0; i < FACE_COUNT; ++i)
        facePaths[i] = root.child(FACE_LABELS[i]).attribute("src").as_string();

    return true;
}

MCubemapAsset::~MCubemapAsset()
{
    releaseCubemap();
}

void MCubemapAsset::deferredAssetLoad(bool forced)
{
    // Skip if already built, unless a forced rebuild was requested
    if (texture && !forced) return;

    // A forced rebuild used to overwrite `texture` and leak the old one.
    releaseCubemap();

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

        // Face entries are reference strings — "guid:<id>" or a bare path.
        const auto faceAsset = assetManager->getAssetFromReference<MTextureAsset>(facePaths[i]);
        if (!faceAsset)
        {
            MERROR(STR("MCubemapAsset: failed to load face '") + FACE_LABELS[i]
                   + "' from reference: " + facePaths[i]);
            return false;
        }
        faceAssets.push_back(faceAsset);
    }

    texture = MCubemapTexture::createCubeMap(faceAssets);
    return texture != nullptr;
}

MTexture* MCubemapAsset::getTexture() { return texture; }

bool MCubemapAsset::dependsOn(const SString& assetPath) const
{
    // Faces may be stored as "guid:<id>", so compare against the resolved
    // asset's path rather than the raw string. Only runs on hot-reload.
    auto* assetManager = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>();
    if (!assetManager) return false;

    for (const auto& face : facePaths)
    {
        if (face == assetPath)
            return true;
        const auto faceAsset = assetManager->getAssetFromReference<MAsset>(face);
        if (faceAsset && faceAsset->getPath() == assetPath)
            return true;
    }
    return false;
}

// Returns the face's reference string — "guid:<id>" or a path.
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

    // Write faces as "guid:<id>". Unresolvable references are kept as-is so a
    // missing texture doesn't silently wipe the face.
    auto* assetManager = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>();
    for (int i = 0; i < FACE_COUNT; ++i)
    {
        if (assetManager && !facePaths[i].empty())
            facePaths[i] = assetManager->toCanonicalReference<MTextureAsset>(facePaths[i]);
        root.append_child(FACE_LABELS[i]).append_attribute("src").set_value(facePaths[i].c_str());
    }

    auto target = MAssetSources::getWritable();
    if (!target)
    {
        MERROR("MCubemapAsset::save — active asset source is read-only: " + path);
        return false;
    }

    std::ostringstream oss;
    doc.save(oss);
    if (!target->writeText(path, SString(oss.str())))
    {
        MERROR("MCubemapAsset::save — failed to write " + path);
        return false;
    }
    return true;
}

bool MCubemapAsset::requestReload()
{
    // Re-read the .skybox file so face-path edits made outside the editor are
    // picked up, then rebuild the GPU cubemap.
    releaseCubemap();
    valid = parseDefinition() && buildCubemap();
    return valid;
}

void MCubemapAsset::releaseCubemap()
{
    // MCubemapTexture has no destructor that calls glDeleteTextures, so the
    // GL handle is released here. If MCubemapTexture ever gains a destructor
    // that deletes its texture, remove the glDeleteTextures call below.
    if (!texture) return;

    unsigned int texId = texture->getTextureID();
    if (texId != 0)
        glDeleteTextures(1, &texId);
    delete texture;
    texture = nullptr;
}
