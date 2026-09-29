//
// assimp_asset_io.h
//
// Assimp IOSystem over an IAssetSource.
//
// Assimp opens files itself, including companion files such as .mtl (OBJ)
// and .bin (glTF). Installing this IO handler on an Assimp::Importer makes
// every one of those opens go through the asset source, so meshes load the
// same way from a project folder or from a package.
//
// Usage:
//     Assimp::Importer importer;
//     importer.SetIOHandler(new MAssimpAssetIOSystem(MAssetSources::getActive()));
//     importer.ReadFile(assetPath, flags);
//
// The Importer takes ownership of the IOSystem and deletes it.
//

#ifndef ASSIMP_ASSET_IO_H
#define ASSIMP_ASSET_IO_H

#include <memory>

#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>

#include "core/engine/assetmanagement/source/asset_source.h"

class MAssimpAssetIOStream final : public Assimp::IOStream
{
public:
    explicit MAssimpAssetIOStream(std::unique_ptr<IAssetStream> inStream)
        : stream(std::move(inStream)) {}

    size_t   Read(void* pvBuffer, size_t pSize, size_t pCount) override;
    size_t   Write(const void*, size_t, size_t) override { return 0; }
    aiReturn Seek(size_t pOffset, aiOrigin pOrigin) override;
    size_t   Tell() const override;
    size_t   FileSize() const override;
    void     Flush() override {}

private:
    std::unique_ptr<IAssetStream> stream;
};

class MAssimpAssetIOSystem final : public Assimp::IOSystem
{
public:
    explicit MAssimpAssetIOSystem(std::shared_ptr<IAssetSource> inSource)
        : source(std::move(inSource)) {}

    bool              Exists(const char* pFile) const override;
    char              getOsSeparator() const override { return '/'; }
    Assimp::IOStream* Open(const char* pFile, const char* pMode = "rb") override;
    void              Close(Assimp::IOStream* pFile) override;

private:
    std::shared_ptr<IAssetSource> source;
};

#endif // ASSIMP_ASSET_IO_H
