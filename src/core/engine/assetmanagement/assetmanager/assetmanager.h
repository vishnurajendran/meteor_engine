//
// Created by ssj5v on 27-09-2024.
//
#pragma once
#ifndef ASSETMANAGER_H
#define ASSETMANAGER_H

#include <map>
#include <memory>
#include <vector>

#include "asset_manager_subsystem.h"
#include "core/engine/assetmanagement/asset/asset_handle.h"
#include "core/engine/subsystem/subsystem_interface.h"
#include "core/object/object.h"

class IDefferedLoadableAsset;
class IAssetSource;
class MAsset;

namespace pugi {
    class xml_document;
}

SCRIPT_BIND_CLASS()
class MAssetManager : public MObject, public IAssetManagerSubsystem
{
    DEFINE_OBJECT_SUBCLASS(MAssetManager)
public:
    // -- Script API --------------------------------------------------------------
    // Static lookups for Lua. They return handles, which resolve by GUID on
    // every get(), never raw pointers - a script can't hold on to an asset
    // that a refresh or reload has since deleted. An unknown path or ID gives
    // a null handle (isValid() == false), not an error.
    //
    //   local clip = MAssetManager.find("assets/sfx/boom.wav")
    //   if clip:isValid() then source:playOneShot(clip) end

    SCRIPT_BIND_FUNC_STATIC()
    static TAssetHandle<MAsset> find(const SString& path);

    SCRIPT_BIND_FUNC_STATIC()
    static TAssetHandle<MAsset> findById(const SString& assetId);

    // "guid:<id>" or a plain path - the format scene files use.
    SCRIPT_BIND_FUNC_STATIC()
    static TAssetHandle<MAsset> findByReference(const SString& reference);

    SCRIPT_BIND_FUNC_STATIC()
    static bool exists(const SString& path);

    // Every loaded asset's path, sorted.
    SCRIPT_BIND_FUNC_STATIC()
    static std::vector<SString> getAssetPaths();

    virtual void refresh();
    virtual void init() override { };
    virtual void cleanup() override;

    // -- Asset source ------------------------------------------------------------
    // Sets where assets are loaded from and makes it the process-wide active
    // source (MAssetSources). Call refresh() afterwards to reload everything.
    // If no source is set, refresh() creates createDefaultSource().
    void setAssetSource(std::shared_ptr<IAssetSource> source);
    [[nodiscard]] std::shared_ptr<IAssetSource> getAssetSource() const { return assetSource; }

    // Builds the standard source for a project:
    //   - project folder (PROJECT_PATH) scanning `projectAssetFolders`
    //   - engine install (ENGINE_PATH) for every path under "meteor_assets/",
    //     scanning "meteor_assets/engine_assets/"
    // When engine and project are the same folder (running from the build
    // output), this is a single directory source scanning both.
    static std::shared_ptr<IAssetSource> createProjectSource(const std::vector<SString>& projectAssetFolders);

    static constexpr const char* ENGINE_ASSET_ROUTE       = "meteor_assets/";
    static constexpr const char* ENGINE_ASSET_SEARCH_PATH = "meteor_assets/engine_assets/";

    // -- Reload ------------------------------------------------------------------
    // Reloads the asset, then rebuilds every loaded asset that depends on it
    // (MAsset::dependsOn), e.g. materials using a reloaded shader.
    virtual bool reloadAsset(MAsset* asset);

    // Every asset path this manager can provide. The editor lists what is
    // loaded (everything); the player lists the whole package, loaded or not.
    [[nodiscard]] virtual std::vector<SString> listAssetPaths() const;

protected:
    // Default: createProjectSource({"assets/"}). The player will override
    // this to return the package source.
    virtual std::shared_ptr<IAssetSource> createDefaultSource();
    IAssetSource& source();

    // Project asset folders used when no project descriptor supplies them.
    std::vector<SString> DEFAULT_PROJECT_ASSET_FOLDERS = {"assets/"};
    std::vector<MAsset*> defferedLoadableAssetList;  // MAsset* - no cast needed, deferredAssetLoad is virtual on MAsset

    static MAssetManager* managerInstance;
    virtual bool loadAsset(SString path);
    virtual void addToDeferredLoadableAssetList(MAsset* asset);

    // Removes the asset from every map and list and deletes it. Does not touch
    // the file. Returns false if the path was not registered.
    virtual bool unregisterAsset(const SString& path);

    // Meta goes through the source: <path>.meta on disk, or the package TOC.
    // createMetaFile() is a no-op on read-only sources.
    void createMetaFile(const SString& filePath);
    bool loadMetaData(const SString& path, pugi::xml_document& metaData);
    bool hasMetaData(const SString& path);

private:
    std::shared_ptr<IAssetSource> assetSource;
};

#endif //ASSETMANAGER_H
