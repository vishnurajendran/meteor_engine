//
// routed_asset_source.h
//
// Sends each asset path to one of several sources by path prefix.
//
// The editor uses it to keep engine and project assets apart without
// changing any asset paths:
//
//     auto routed = std::make_shared<MRoutedAssetSource>(projectSource);
//     routed->addRoute("meteor_assets/", engineSource);
//
//   "meteor_assets/engine_assets/icons/sun.png"  -> engine install
//   "meteor_assets/shader_base/standard.glsl"    -> engine install
//   "assets/scenes/level.scml"                   -> project
//
// Longest matching prefix wins; anything unmatched goes to the fallback.
// enumerate() lists every source, each restricted to the paths it owns, so a
// project that happens to contain its own meteor_assets/ folder is not listed
// twice. Writes go to the owning source if it is writable.
//
// Thread-safe for reads once routes are set up (routes are fixed after
// construction by convention; do not add routes while assets load).
//

#ifndef ROUTED_ASSET_SOURCE_H
#define ROUTED_ASSET_SOURCE_H

#include <memory>
#include <vector>

#include "core/engine/assetmanagement/source/asset_source.h"

class MRoutedAssetSource final : public IWritableAssetSource
{
public:
    explicit MRoutedAssetSource(std::shared_ptr<IAssetSource> fallback);

    // `prefix` is an asset-path prefix such as "meteor_assets/". A trailing
    // '/' is added if missing, so "meteor_assets" does not match
    // "meteor_assets_old/...".
    void addRoute(const SString& prefix, std::shared_ptr<IAssetSource> source);

    // The source that owns `path` (never null).
    [[nodiscard]] IAssetSource& sourceFor(const SString& path) const;

    // Fallback first, then routes in the order added.
    [[nodiscard]] std::vector<std::shared_ptr<IAssetSource>> getSources() const;

    // -- IAssetSource -----------------------------------------------------------
    SString getDebugName() const override;

    bool exists(const SString& path) const override;
    void enumerate(const std::function<void(const SAssetSourceEntry&)>& fn) const override;

    bool readBytes(const SString& path, std::vector<uint8_t>& out) const override;
    bool readText(const SString& path, SString& out) const override;
    std::unique_ptr<IAssetStream> openStream(const SString& path) const override;

    bool hasMeta(const SString& path) const override;
    bool readMeta(const SString& path, pugi::xml_document& out) const override;

    SString getDiskPath(const SString& path) const override;

    // -- IWritableAssetSource ---------------------------------------------------
    // Each fails (returns false, logs) if the owning source is read-only.
    bool writeBytes(const SString& path, const void* data, size_t size) override;
    bool writeMeta(const SString& path, const pugi::xml_document& doc) override;
    bool remove(const SString& path) override;
    bool createDirectory(const SString& path) override;
    bool removeDirectory(const SString& path) override;

private:
    struct SRoute
    {
        std::string                   prefix;   // normalized, ends with '/'
        std::shared_ptr<IAssetSource> source;
    };

    const SRoute*         findRoute(const std::string& path) const;
    IWritableAssetSource* writableFor(const SString& path, const char* op) const;
    static bool           startsWith(const std::string& s, const std::string& prefix);

    std::shared_ptr<IAssetSource> fallback;
    std::vector<SRoute>           routes;
};

#endif // ROUTED_ASSET_SOURCE_H
