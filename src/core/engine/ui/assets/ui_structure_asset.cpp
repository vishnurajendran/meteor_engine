//
// ui_structure_asset.cpp
//

#include "ui_structure_asset.h"

#include <regex>
#include <string>

#include "core/engine/assetmanagement/assetmanager/asset_manager_subsystem.h"
#include "core/engine/assetmanagement/source/asset_sources.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/utils/fileio.h"
#include "core/utils/logger.h"

namespace
{
    // Case-insensitive search for an ASCII tag fragment.
    size_t findNoCase(const std::string& text, const std::string& needle, size_t from = 0)
    {
        if (needle.empty() || text.size() < needle.size())
            return std::string::npos;
        for (size_t i = from; i + needle.size() <= text.size(); ++i)
        {
            bool match = true;
            for (size_t j = 0; j < needle.size() && match; ++j)
                match = std::tolower(static_cast<unsigned char>(text[i + j])) == needle[j];
            if (match)
                return i;
        }
        return std::string::npos;
    }

    size_t rfindNoCase(const std::string& text, const std::string& needle)
    {
        size_t found = std::string::npos;
        for (size_t at = findNoCase(text, needle); at != std::string::npos; at = findNoCase(text, needle, at + 1))
            found = at;
        return found;
    }
}

MUIStructureAsset::MUIStructureAsset(const SString& path) : MAsset(path)
{
    name  = FileIO::getFileName(path);
    valid = load();
}

bool MUIStructureAsset::requestReload()
{
    valid = load();
    return valid;
}

bool MUIStructureAsset::load()
{
    SString text;
    if (!MAssetSources::getActive()->readText(path, text))
    {
        MERROR("MUIStructureAsset: failed to read " + path);
        return false;
    }
    source = text;
    parseWidgetReferences();
    return true;
}

SString MUIStructureAsset::getBodyContent() const
{
    const std::string& text = source.str();
    const size_t bodyTag = findNoCase(text, "<body");
    if (bodyTag == std::string::npos)
        return source;

    const size_t open = text.find('>', bodyTag);
    if (open == std::string::npos)
        return SString();
    // <body/> - nothing inside.
    if (open > 0 && text[open - 1] == '/')
        return SString();

    const size_t close = rfindNoCase(text, "</body");
    if (close == std::string::npos || close < open)
        return SString(text.substr(open + 1));
    return SString(text.substr(open + 1, close - open - 1));
}

void MUIStructureAsset::parseWidgetReferences()
{
    widgetReferences.clear();

    // <widget ... src="..."> - attribute in either quote style, anywhere in the tag.
    static const std::regex pattern(R"(<\s*widget\b[^>]*?\bsrc\s*=\s*("|')(.*?)\1)",
                                    std::regex::icase | std::regex::ECMAScript);
    const std::string& text = source.str();
    for (auto it = std::sregex_iterator(text.begin(), text.end(), pattern); it != std::sregex_iterator(); ++it)
    {
        const std::smatch& match = *it;
        SWidgetReference ref;
        ref.src         = SString(match[2].str());
        ref.isGuid      = IAssetManagerSubsystem::isGuidReference(ref.src);
        ref.valueOffset = static_cast<size_t>(match.position(2));
        ref.valueLength = static_cast<size_t>(match.length(2));
        widgetReferences.push_back(ref);
    }
}

int MUIStructureAsset::convertWidgetPathsToGuids()
{
    auto* manager = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>();
    if (!manager)
        return -1;

    // Back to front, so earlier offsets stay valid while later values change length.
    std::string text = source.str();
    int converted = 0;
    for (auto it = widgetReferences.rbegin(); it != widgetReferences.rend(); ++it)
    {
        if (it->isGuid)
            continue;
        const SString canonical = manager->toCanonicalReference<MAsset>(it->src);
        if (canonical == it->src)
        {
            MWARN(SString::format("'{0}': <widget src=\"{1}\"> does not resolve to an asset - left as it is",
                                  path, it->src));
            continue;
        }
        text.replace(it->valueOffset, it->valueLength, canonical.str());
        ++converted;
    }

    if (converted == 0)
        return 0;

    auto target = MAssetSources::getWritable();
    if (!target || !target->writeText(path, SString(text)))
    {
        MERROR("MUIStructureAsset: could not save " + path);
        return -1;
    }

    source = SString(text);
    parseWidgetReferences();
    return converted;
}
