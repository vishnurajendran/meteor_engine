//
// asset_creator.cpp
//

#include "asset_creator.h"

std::vector<MAssetCreatorRegistry::SEntry>& MAssetCreatorRegistry::entries()
{
    static std::vector<SEntry> list;
    return list;
}

void MAssetCreatorRegistry::registerCreator(uint64_t assetTypeHash, const char* assetTypeName,
                                            std::unique_ptr<IAssetCreator> creator)
{
    if (!creator)
        return;
    for (auto& entry : entries())
    {
        // A later registration replaces the earlier one.
        if (entry.hash == assetTypeHash)
        {
            entry.creator  = std::move(creator);
            entry.typeName = assetTypeName;
            return;
        }
    }
    entries().push_back({ assetTypeHash, assetTypeName, std::move(creator) });
}

IAssetCreator* MAssetCreatorRegistry::find(uint64_t assetTypeHash)
{
    for (const auto& entry : entries())
        if (entry.hash == assetTypeHash)
            return entry.creator.get();
    return nullptr;
}

std::vector<IAssetCreator*> MAssetCreatorRegistry::getMenuCreators()
{
    std::vector<IAssetCreator*> creators;
    for (const auto& entry : entries())
        if (entry.creator && !entry.creator->getMenuPath().empty())
            creators.push_back(entry.creator.get());
    return creators;
}
