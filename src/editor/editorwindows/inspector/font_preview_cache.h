//
// font_preview_cache.h
//
// ImGui fonts for previewing font assets in the inspector. A font is added to
// ImGui's atlas the first time it is previewed and reused after that; a
// reloaded font (new generation) is added again. Adding to the atlas is only
// allowed between frames, so get() queues the request and processPending()
// - called by the editor window before each new ImGui frame - does it.
//

#ifndef FONT_PREVIEW_CACHE_H
#define FONT_PREVIEW_CACHE_H

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct ImFont;
class MFontAsset;

class MFontPreviewCache
{
public:
    // The sample line drawn in every size.
    static constexpr const char* PREVIEW_TEXT = "The quick brown fox jumps over the lazy dog";
    static constexpr std::array<float, 3> PREVIEW_SIZES = { 14.0f, 24.0f, 36.0f };

    using FontSet = std::array<ImFont*, PREVIEW_SIZES.size()>;

    // The preview fonts for this asset, or nullptr until they are in the atlas
    // (normally the next frame).
    static const FontSet* get(const MFontAsset* font);

    // Adds requested fonts to the atlas. Between frames only.
    static void processPending();

private:
    struct SEntry
    {
        uint64_t generation = 0;
        std::shared_ptr<const std::vector<uint8_t>> data;   // the atlas reads from it
        FontSet fonts{};
        bool    ready = false;
    };

    static std::unordered_map<std::string, SEntry>& entries();
    static std::vector<std::string>& pending();
};

#endif // FONT_PREVIEW_CACHE_H
