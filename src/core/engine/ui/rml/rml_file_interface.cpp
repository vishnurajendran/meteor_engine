//
// rml_file_interface.cpp
//

#include "rml_file_interface.h"

#include <cstdio>
#include <memory>

#include "core/engine/assetmanagement/assetmanager/asset_manager_subsystem.h"
#include "core/engine/assetmanagement/source/asset_sources.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/utils/logger.h"

namespace
{
    struct SOpenFile
    {
        std::unique_ptr<IAssetStream> stream;
    };

    SOpenFile* toFile(Rml::FileHandle handle) { return reinterpret_cast<SOpenFile*>(handle); }
}

SString MRmlFileInterface::resolvePath(const Rml::String& path)
{
    const SString reference(path);
    if (IAssetManagerSubsystem::isGuidReference(reference))
    {
        auto* manager = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>();
        const MAsset* asset = manager ? manager->getAssetFromReference<MAsset>(reference).get() : nullptr;
        return asset ? asset->getPath() : SString();
    }
    return MAssetPath::normalize(reference);
}

Rml::FileHandle MRmlFileInterface::Open(const Rml::String& path)
{
    const SString assetPath = resolvePath(path);
    if (assetPath.empty())
        return 0;

    auto stream = MAssetSources::getActive()->openStream(assetPath);
    if (!stream)
        return 0;   // RmlUi logs the failure with the document that asked

    auto* file = new SOpenFile{ std::move(stream) };
    return reinterpret_cast<Rml::FileHandle>(file);
}

void MRmlFileInterface::Close(Rml::FileHandle file)
{
    delete toFile(file);
}

size_t MRmlFileInterface::Read(void* buffer, size_t size, Rml::FileHandle file)
{
    auto* open = toFile(file);
    return open && open->stream ? open->stream->read(buffer, size) : 0;
}

bool MRmlFileInterface::Seek(Rml::FileHandle file, long offset, int origin)
{
    auto* open = toFile(file);
    if (!open || !open->stream)
        return false;

    int64_t base = 0;
    switch (origin)
    {
        case SEEK_SET: base = 0; break;
        case SEEK_CUR: base = static_cast<int64_t>(open->stream->tell()); break;
        case SEEK_END: base = static_cast<int64_t>(open->stream->size()); break;
        default:       return false;
    }
    const int64_t target = base + offset;
    if (target < 0)
        return false;
    return open->stream->seek(static_cast<uint64_t>(target));
}

size_t MRmlFileInterface::Tell(Rml::FileHandle file)
{
    auto* open = toFile(file);
    return open && open->stream ? static_cast<size_t>(open->stream->tell()) : 0;
}

size_t MRmlFileInterface::Length(Rml::FileHandle file)
{
    auto* open = toFile(file);
    return open && open->stream ? static_cast<size_t>(open->stream->size()) : 0;
}
