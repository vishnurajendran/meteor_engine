//
// asset_sources.cpp
//

#include "core/engine/assetmanagement/source/asset_sources.h"

#include <mutex>

#include "core/engine/assetmanagement/source/directory_asset_source.h"
#include "core/utils/logger.h"

namespace
{
std::mutex&                    sourceMutex() { static std::mutex m; return m; }
std::shared_ptr<IAssetSource>& sourceSlot()  { static std::shared_ptr<IAssetSource> s; return s; }
} // namespace

std::shared_ptr<IAssetSource> MAssetSources::getActive()
{
    std::lock_guard lock(sourceMutex());
    auto& slot = sourceSlot();
    if (!slot)
        slot = std::make_shared<MDirectoryAssetSource>(SString(""), std::vector<SString>{});
    return slot;
}

std::shared_ptr<IWritableAssetSource> MAssetSources::getWritable()
{
    auto active = getActive();
    if (auto* writable = active->asWritable())
        return std::shared_ptr<IWritableAssetSource>(active, writable);   // aliasing ctor
    return nullptr;
}

void MAssetSources::setActive(std::shared_ptr<IAssetSource> source)
{
    if (source)
        MLOG(SString("MAssetSources:: active source -> ") + source->getDebugName());

    std::lock_guard lock(sourceMutex());
    sourceSlot() = std::move(source);
}
