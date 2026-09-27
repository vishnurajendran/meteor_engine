//
// directory_asset_source.cpp
//

#include "core/engine/assetmanagement/source/directory_asset_source.h"

#include <fstream>
#include <sstream>
#include <system_error>

#include "core/utils/logger.h"
#include "pugixml.hpp"

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// File-backed stream. One std::ifstream per stream, so streams are
// independent of each other across threads.
// ---------------------------------------------------------------------------

namespace
{
class MFileAssetStream final : public IAssetStream
{
public:
    bool open(const fs::path& p)
    {
        file.open(p, std::ios::binary | std::ios::in);
        if (!file.is_open()) return false;

        file.seekg(0, std::ios::end);
        const auto end = file.tellg();
        file.seekg(0, std::ios::beg);
        if (end < 0) return false;

        fileSize = static_cast<uint64_t>(end);
        return true;
    }

    size_t read(void* dst, size_t bytes) override
    {
        if (bytes == 0 || cursor >= fileSize) return 0;

        file.read(static_cast<char*>(dst), static_cast<std::streamsize>(bytes));
        const auto got = static_cast<size_t>(file.gcount());
        cursor += got;

        // Reading up to EOF sets eof/fail bits; clear them so later seeks work.
        if (!file) file.clear();
        return got;
    }

    bool seek(uint64_t offset) override
    {
        if (offset > fileSize) return false;
        file.clear();
        file.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
        if (!file) { file.clear(); return false; }
        cursor = offset;
        return true;
    }

    uint64_t tell() const override { return cursor; }
    uint64_t size() const override { return fileSize; }

private:
    std::ifstream file;
    uint64_t      fileSize = 0;
    uint64_t      cursor   = 0;
};
} // namespace

// ---------------------------------------------------------------------------
// Construction / path mapping
// ---------------------------------------------------------------------------

MDirectoryAssetSource::MDirectoryAssetSource(const SString& root, std::vector<SString> inSearchPaths)
    : rootPath(root.empty() ? fs::path() : fs::path(root.str()))
    , searchPaths(std::move(inSearchPaths))
{
}

SString MDirectoryAssetSource::getDebugName() const
{
    return SString("Directory(") + SString(rootPath.empty() ? std::string(".") : rootPath.generic_string()) + ")";
}

fs::path MDirectoryAssetSource::resolve(const SString& path) const
{
    fs::path p(path.str());
    if (p.is_absolute() || rootPath.empty())
        return p;
    return rootPath / p;
}

SString MDirectoryAssetSource::toAssetPath(const fs::path& p) const
{
    if (rootPath.empty())
        return MAssetPath::normalize(SString(p.generic_string()));

    const fs::path rel = p.lexically_relative(rootPath);
    return MAssetPath::normalize(SString((rel.empty() ? p : rel).generic_string()));
}

SString MDirectoryAssetSource::getDiskPath(const SString& path) const
{
    std::error_code ec;
    fs::path abs = fs::absolute(resolve(path), ec);
    if (ec) return SString(resolve(path).generic_string());
    return SString(abs.lexically_normal().generic_string());
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

bool MDirectoryAssetSource::exists(const SString& path) const
{
    std::error_code ec;
    return fs::exists(resolve(path), ec);
}

void MDirectoryAssetSource::enumerate(const std::function<void(const SAssetSourceEntry&)>& fn) const
{
    for (const auto& searchPath : searchPaths)
    {
        const fs::path dir = resolve(searchPath);
        std::error_code ec;
        if (!fs::exists(dir, ec)) continue;

        fs::recursive_directory_iterator it(dir, fs::directory_options::skip_permission_denied, ec);
        if (ec)
        {
            MWARN(SString("MDirectoryAssetSource:: cannot enumerate ") + searchPath);
            continue;
        }

        for (const auto& entry : it)
        {
            SAssetSourceEntry e;
            e.isDirectory = entry.is_directory(ec);
            e.path        = toAssetPath(entry.path());

            if (!e.isDirectory && MAssetPath::isMetaPath(e.path))
                continue;

            fn(e);
        }
    }
}

// ---------------------------------------------------------------------------
// Reading
// ---------------------------------------------------------------------------

bool MDirectoryAssetSource::readBytes(const SString& path, std::vector<uint8_t>& out) const
{
    MFileAssetStream stream;
    if (!stream.open(resolve(path)))
        return false;
    return stream.readAll(out);
}

std::unique_ptr<IAssetStream> MDirectoryAssetSource::openStream(const SString& path) const
{
    auto stream = std::make_unique<MFileAssetStream>();
    if (!stream->open(resolve(path)))
        return nullptr;
    return stream;
}

bool MDirectoryAssetSource::hasMeta(const SString& path) const
{
    return exists(MAssetPath::metaPathFor(path));
}

bool MDirectoryAssetSource::readMeta(const SString& path, pugi::xml_document& out) const
{
    std::vector<uint8_t> bytes;
    if (!readBytes(MAssetPath::metaPathFor(path), bytes))
        return false;

    const auto res = out.load_buffer(bytes.data(), bytes.size());
    return res.status == pugi::status_ok;
}

// ---------------------------------------------------------------------------
// Writing
// ---------------------------------------------------------------------------

bool MDirectoryAssetSource::writeBytes(const SString& path, const void* data, size_t size)
{
    const fs::path p = resolve(path);

    std::error_code ec;
    if (p.has_parent_path())
        fs::create_directories(p.parent_path(), ec);

    std::ofstream ofs(p, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!ofs.is_open())
    {
        MERROR(SString("MDirectoryAssetSource:: cannot open for write: ") + path);
        return false;
    }

    if (size > 0)
        ofs.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));

    if (!ofs)
    {
        MERROR(SString("MDirectoryAssetSource:: write failed: ") + path);
        return false;
    }
    return true;
}

bool MDirectoryAssetSource::writeMeta(const SString& path, const pugi::xml_document& doc)
{
    std::ostringstream oss;
    doc.save(oss);
    const std::string s = oss.str();
    return writeBytes(MAssetPath::metaPathFor(path), s.data(), s.size());
}

bool MDirectoryAssetSource::remove(const SString& path)
{
    std::error_code ec;
    const bool removed = fs::remove(resolve(path), ec);
    if (ec)
    {
        MWARN(SString::format("MDirectoryAssetSource:: failed to delete {0} ({1})",
                              path, SString(ec.message())));
    }

    std::error_code metaEc;
    fs::remove(resolve(MAssetPath::metaPathFor(path)), metaEc);
    return removed && !ec;
}

bool MDirectoryAssetSource::createDirectory(const SString& path)
{
    std::error_code ec;
    fs::create_directories(resolve(path), ec);
    if (ec)
    {
        MERROR(SString::format("MDirectoryAssetSource:: failed to create directory {0} ({1})",
                               path, SString(ec.message())));
        return false;
    }
    return true;
}

bool MDirectoryAssetSource::removeDirectory(const SString& path)
{
    std::error_code ec;
    fs::remove_all(resolve(path), ec);
    if (ec)
    {
        MWARN(SString::format("MDirectoryAssetSource:: failed to delete directory {0} ({1})",
                              path, SString(ec.message())));
    }

    std::error_code metaEc;
    fs::remove(resolve(MAssetPath::metaPathFor(path)), metaEc);
    return !ec;
}
