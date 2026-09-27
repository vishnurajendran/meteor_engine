//
// asset_source.cpp
//

#include "core/engine/assetmanagement/source/asset_source.h"

#include <algorithm>
#include <filesystem>
#include <string>

// ---------------------------------------------------------------------------
// MAssetPath
// ---------------------------------------------------------------------------

SString MAssetPath::normalize(const SString& path)
{
    std::string s = path.str();
    if (s.empty()) return path;

    std::replace(s.begin(), s.end(), '\\', '/');

    // lexically_normal() folds "a/./b" and "a/x/../b" without touching disk.
    std::string out = std::filesystem::path(s).lexically_normal().generic_string();

    while (out.size() >= 2 && out[0] == '.' && out[1] == '/')
        out.erase(0, 2);

    // lexically_normal keeps a trailing slash for directories ("a/b/"); drop it
    // so directory keys match the ones the editor builds.
    if (out.size() > 1 && out.back() == '/')
        out.pop_back();

    return SString(out);
}

bool MAssetPath::isMetaPath(const SString& path)
{
    const std::string& s = path.str();
    static const std::string ext = ".meta";
    return s.size() >= ext.size() && s.compare(s.size() - ext.size(), ext.size(), ext) == 0;
}

// ---------------------------------------------------------------------------
// Default implementations
// ---------------------------------------------------------------------------

bool IAssetSource::readText(const SString& path, SString& out) const
{
    std::vector<uint8_t> bytes;
    if (!readBytes(path, bytes))
        return false;

    out = SString(std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size()));
    return true;
}

bool IWritableAssetSource::writeText(const SString& path, const SString& text)
{
    const std::string& s = text.str();
    return writeBytes(path, s.data(), s.size());
}

bool MAssetPath::isIgnored(const SString& path)
{
    if (isMetaPath(path)) return true;

    const std::string& s = path.str();
    const size_t slash = s.find_last_of('/');
    const size_t nameStart = (slash == std::string::npos) ? 0 : slash + 1;
    return nameStart < s.size() && s[nameStart] == '~';
}
