//
// asset_source.h
//
// Where asset bytes come from.
//
//   IAssetSource          read-only; what runtime asset code depends on.
//                         Implemented by MDirectoryAssetSource (editor / dev)
//                         and, later, the package source (player builds).
//
//   IWritableAssetSource  editor-only extension; saving, creating, deleting.
//                         Runtime code never needs it, so player builds can
//                         run entirely off a read-only package.
//
// Paths
// -----
// All paths are asset paths: forward slashes, relative to the source root,
// exactly the strings used as keys in the asset manager
// (e.g. "assets/textures/brick.png"). Use MAssetPath::normalize() on anything
// that came from a third-party library or from user input.
//
// Threading
// ---------
// Every const method must be safe to call from any thread. Audio decoding and
// streaming happen on miniaudio's job threads.
//

#ifndef ASSET_SOURCE_H
#define ASSET_SOURCE_H

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include "core/engine/assetmanagement/source/asset_stream.h"
#include "core/utils/sstring.h"

namespace pugi { class xml_document; }

// ---------------------------------------------------------------------------
// Path helpers
// ---------------------------------------------------------------------------

struct MAssetPath
{
    // "assets\\a\\.\\b\\..\\c.png" -> "assets/a/c.png". Strips a leading "./".
    static SString normalize(const SString& path);

    static SString metaPathFor(const SString& path) { return path + ".meta"; }
    static bool    isMetaPath(const SString& path);

    // Files the asset system skips entirely: .meta sidecars and any file whose
    // name starts with '~' (editor temp / backup files).
    static bool    isIgnored(const SString& path);
};

// ---------------------------------------------------------------------------
// Read-only source
// ---------------------------------------------------------------------------

struct SAssetSourceEntry
{
    SString path;          // normalized asset path
    bool    isDirectory = false;
};

class IWritableAssetSource;

class IAssetSource
{
public:
    virtual ~IAssetSource() = default;

    // Human-readable, for logs ("Directory(C:/proj)", "Package(game.mpak)").
    virtual SString getDebugName() const = 0;

    // -- Queries ---------------------------------------------------------------
    virtual bool exists(const SString& path) const = 0;

    // Calls fn once per file and directory. .meta sidecars are NOT reported.
    virtual void enumerate(const std::function<void(const SAssetSourceEntry&)>& fn) const = 0;

    // -- Reading ---------------------------------------------------------------
    virtual bool readBytes(const SString& path, std::vector<uint8_t>& out) const = 0;

    // Default implementation goes through readBytes().
    virtual bool readText(const SString& path, SString& out) const;

    // Returns nullptr if the path does not exist.
    virtual std::unique_ptr<IAssetStream> openStream(const SString& path) const = 0;

    // -- Metadata (GUID + import settings) --------------------------------------
    // The directory source reads "<path>.meta"; the package source reads the
    // meta blob stored in its table of contents.
    virtual bool hasMeta(const SString& path) const = 0;
    virtual bool readMeta(const SString& path, pugi::xml_document& out) const = 0;

    // -- Editor conveniences -------------------------------------------------
    // Absolute path on disk, for "open in external program". Empty when the
    // asset has no backing file (package).
    virtual SString getDiskPath(const SString& /*path*/) const { return {}; }

    // nullptr for read-only sources.
    virtual IWritableAssetSource*       asWritable()       { return nullptr; }
    virtual const IWritableAssetSource* asWritable() const { return nullptr; }
};

// ---------------------------------------------------------------------------
// Writable source (editor)
// ---------------------------------------------------------------------------

class IWritableAssetSource : public IAssetSource
{
public:
    IWritableAssetSource*       asWritable()       override { return this; }
    const IWritableAssetSource* asWritable() const override { return this; }

    // Creates parent directories as needed. Overwrites existing files.
    virtual bool writeBytes(const SString& path, const void* data, size_t size) = 0;
    virtual bool writeText(const SString& path, const SString& text);
    virtual bool writeMeta(const SString& path, const pugi::xml_document& doc) = 0;

    // Removes the file and its .meta sidecar.
    virtual bool remove(const SString& path) = 0;

    virtual bool createDirectory(const SString& path) = 0;
    // Recursive. Also removes the directory's .meta sidecar.
    virtual bool removeDirectory(const SString& path) = 0;
};

#endif // ASSET_SOURCE_H
