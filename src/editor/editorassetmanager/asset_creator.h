//
// asset_creator.h
//
// A typed way to create assets from code - the Create menu uses it, and so do
// creators that build several assets (Create > UI Widget makes a .rml, an
// .rcss and a .lua, then a .muia that references their GUIDs).
//
//   struct SMyInitData : SAssetInitialisationData { SString title; };
//
//   class MMyCreator : public TAssetCreator<MMyAsset, SMyInitData>
//   {
//   public:
//       SString getExtension()   const override { return ".my"; }
//       SString getDisplayName() const override { return "My Asset"; }
//       SString getMenuPath()    const override { return "Misc/My Asset"; }   // empty = code only
//   protected:
//       SString createTyped(MEditorAssetManager& manager, const SString& directory,
//                           const SString& name, const SMyInitData& data) override;
//   };
//   REGISTER_ASSET_CREATOR(MMyAsset, MMyCreator)
//
//   auto handle = editorAssetManager->createAsset<MMyAsset>("assets/things", "Thing", SMyInitData{ .title = "x" });
//
// createTyped writes the file(s) and imports them (MEditorAssetManager::
// writeAndImport) and returns the new asset's path. The handle createAsset
// returns resolves by GUID, so the next creator can reference it straight away.
//
// Editor only (src/editor/), so it never reaches the player.
//
// The text-template registry (asset_template_registry.h) stays for simple
// one-file templates; shader and material templates can move here later.
//

#ifndef ASSET_CREATOR_H
#define ASSET_CREATOR_H

#include <cstdint>
#include <memory>
#include <type_traits>
#include <typeinfo>
#include <vector>

#include "core/utils/logger.h"
#include "core/utils/sstring.h"

class MEditorAssetManager;

// Base of every creator's options. Pass a derived struct; a plain
// SAssetInitialisationData means "defaults".
struct SAssetInitialisationData
{
    virtual ~SAssetInitialisationData() = default;
};

class IAssetCreator
{
public:
    virtual ~IAssetCreator() = default;

    // ".rml", with the dot. Empty for creators that make several files.
    [[nodiscard]] virtual SString getExtension() const = 0;
    // "UI Widget", for menus and messages.
    [[nodiscard]] virtual SString getDisplayName() const = 0;
    // Create-menu path, '/' separates submenus. Empty: not in the menu.
    [[nodiscard]] virtual SString getMenuPath() const { return {}; }
    // Name pre-filled in the create dialog.
    [[nodiscard]] virtual SString getDefaultName() const { return "New_Asset"; }

    // Writes and imports the asset. `data` may be nullptr (defaults). Returns
    // the created asset's path, or empty (with the reason logged).
    virtual SString create(MEditorAssetManager& manager, const SString& directory, const SString& name,
                           const SAssetInitialisationData* data) = 0;
};

// Checks the options' type once, then hands createTyped the right struct.
template <typename TAsset, typename TData = SAssetInitialisationData>
class TAssetCreator : public IAssetCreator
{
    static_assert(std::is_base_of_v<SAssetInitialisationData, TData>, "TData must derive from SAssetInitialisationData");

public:
    SString create(MEditorAssetManager& manager, const SString& directory, const SString& name,
                   const SAssetInitialisationData* data) final
    {
        static const TData defaults{};
        const TData* typed = &defaults;
        if (data && typeid(*data) != typeid(SAssetInitialisationData))
        {
            typed = dynamic_cast<const TData*>(data);
            if (!typed)
            {
                MERROR(SString::format("Create {0}: wrong initialisation data ({1})", getDisplayName(), SString(typeid(*data).name())));
                return {};
            }
        }
        return createTyped(manager, directory, name, *typed);
    }

protected:
    virtual SString createTyped(MEditorAssetManager& manager, const SString& directory, const SString& name,
                                const TData& data) = 0;
};

// Keyed by the asset class's staticTypeInfo() hash. Construct-on-first-use,
// so creators can register from static initialisers in any order.
class MAssetCreatorRegistry
{
public:
    static void registerCreator(uint64_t assetTypeHash, const char* assetTypeName, std::unique_ptr<IAssetCreator> creator);
    [[nodiscard]] static IAssetCreator* find(uint64_t assetTypeHash);
    // Creators with a menu path, in registration order.
    [[nodiscard]] static std::vector<IAssetCreator*> getMenuCreators();

private:
    struct SEntry
    {
        uint64_t hash = 0;
        const char* typeName = "";
        std::unique_ptr<IAssetCreator> creator;
    };
    static std::vector<SEntry>& entries();
};

#define METEOR_ASSET_CREATOR_CONCAT_INNER(a, b) a##b
#define METEOR_ASSET_CREATOR_CONCAT(a, b) METEOR_ASSET_CREATOR_CONCAT_INNER(a, b)

// Registers CreatorType as the creator of AssetType. Use at namespace scope in a .cpp.
#define REGISTER_ASSET_CREATOR(AssetType, CreatorType)                                                   \
    namespace {                                                                                          \
    const bool METEOR_ASSET_CREATOR_CONCAT(g_assetCreatorRegistered_, __LINE__) = []() {                 \
        MAssetCreatorRegistry::registerCreator(AssetType::staticTypeInfo().hash,                         \
                                               AssetType::staticTypeInfo().name,                         \
                                               std::make_unique<CreatorType>());                         \
        return true;                                                                                     \
    }();                                                                                                 \
    }

#endif // ASSET_CREATOR_H
