//
// font_asset.h
//
// A font file (.ttf / .otf) as an asset. Knows nothing about who renders it:
// the UI registers it with RmlUi, the editor previews it with ImGui.
//
// The file's bytes stay in memory for the asset's lifetime - renderers draw
// glyphs from them for as long as the face is registered. A reload swaps in
// a new buffer (getGeneration() moves); holders of the old shared buffer keep
// it alive as long as they need it.
//
// Import settings, in the .meta:
//   <fontImport>
//       <familyOverride></familyOverride>   empty = the file's own family
//       <fallback>false</fallback>          use for glyphs other fonts lack
//   </fontImport>
//

#ifndef FONT_ASSET_H
#define FONT_ASSET_H

#include <cstdint>
#include <memory>
#include <vector>

#include "core/engine/assetmanagement/asset/asset.h"

namespace pugi { class xml_node; }

SCRIPT_BIND_CLASS()
class MFontAsset : public MAsset
{
    DEFINE_OBJECT_SUBCLASS(MFontAsset)
public:
    explicit MFontAsset(const SString& path);
    MFontAsset(const SString& path, const pugi::xml_node& importSettings);

    // The family documents name it by: the override if set, else the file's.
    SCRIPT_BIND_FUNC()
    SString getFamily() const;

    // As the file declares it (typographic family, else family).
    SCRIPT_BIND_FUNC()
    SString getFileFamily() const { return fileFamily; }

    // 100-900 (400 regular, 700 bold).
    SCRIPT_BIND_FUNC()
    int getWeight() const { return weight; }

    SCRIPT_BIND_FUNC()
    bool isItalic() const { return italic; }

    // "Bold Italic", for display.
    SCRIPT_BIND_FUNC()
    SString getStyleName() const { return styleName; }

    // True when the file's tables could not be read; family is then the file
    // name and weight 400.
    [[nodiscard]] bool usedFallbackMetadata() const { return metadataFallback; }

    // The whole file. Never null once loaded.
    [[nodiscard]] const std::vector<uint8_t>& getData() const { return *data; }
    [[nodiscard]] std::shared_ptr<const std::vector<uint8_t>> getSharedData() const { return data; }

    // Changes on every (re)load.
    [[nodiscard]] uint64_t getGeneration() const { return generation; }

    // -- Import settings ---------------------------------------------------------
    [[nodiscard]] const SString& getFamilyOverride() const { return familyOverride; }
    [[nodiscard]] bool isFallback() const { return fallback; }
    void setFamilyOverride(const SString& family) { familyOverride = family; }
    void setFallback(bool isFallbackFace) { fallback = isFallbackFace; }
    void loadImportSettings(const pugi::xml_node& node);
    bool saveImportSettings();

    bool requestReload() override;

private:
    bool load();

    std::shared_ptr<const std::vector<uint8_t>> data = std::make_shared<std::vector<uint8_t>>();
    uint64_t generation = 0;

    SString fileFamily;
    SString styleName = "Regular";
    int     weight = 400;
    bool    italic = false;
    bool    metadataFallback = false;

    SString familyOverride;
    bool    fallback = false;
};

#endif // FONT_ASSET_H
