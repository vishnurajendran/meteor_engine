//
// routed_asset_source.cpp
//

#include "core/engine/assetmanagement/source/routed_asset_source.h"

#include "core/utils/logger.h"

MRoutedAssetSource::MRoutedAssetSource(std::shared_ptr<IAssetSource> inFallback)
    : fallback(std::move(inFallback))
{
}

void MRoutedAssetSource::addRoute(const SString& prefix, std::shared_ptr<IAssetSource> source)
{
    if (!source) return;

    std::string p = MAssetPath::normalize(prefix).str();
    if (p.empty()) return;
    if (p.back() != '/') p.push_back('/');

    routes.push_back({ p, std::move(source) });
}

bool MRoutedAssetSource::startsWith(const std::string& s, const std::string& prefix)
{
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

const MRoutedAssetSource::SRoute* MRoutedAssetSource::findRoute(const std::string& path) const
{
    const SRoute* best = nullptr;
    for (const auto& r : routes)
    {
        // Match "meteor_assets/..." and the bare directory "meteor_assets".
        const bool match = startsWith(path, r.prefix) ||
                           path == r.prefix.substr(0, r.prefix.size() - 1);
        if (match && (!best || r.prefix.size() > best->prefix.size()))
            best = &r;
    }
    return best;
}

IAssetSource& MRoutedAssetSource::sourceFor(const SString& path) const
{
    const SRoute* r = findRoute(MAssetPath::normalize(path).str());
    return r ? *r->source : *fallback;
}

std::vector<std::shared_ptr<IAssetSource>> MRoutedAssetSource::getSources() const
{
    std::vector<std::shared_ptr<IAssetSource>> out;
    out.push_back(fallback);
    for (const auto& r : routes)
        out.push_back(r.source);
    return out;
}

SString MRoutedAssetSource::getDebugName() const
{
    SString name = SString("Routed(") + fallback->getDebugName();
    for (const auto& r : routes)
        name += SString(", ") + SString(r.prefix) + " -> " + r.source->getDebugName();
    return name + ")";
}

// ---------------------------------------------------------------------------
// Reading
// ---------------------------------------------------------------------------

bool MRoutedAssetSource::exists(const SString& path) const
{
    return sourceFor(path).exists(path);
}

void MRoutedAssetSource::enumerate(const std::function<void(const SAssetSourceEntry&)>& fn) const
{
    // Fallback: everything that no route claims.
    fallback->enumerate([&](const SAssetSourceEntry& e)
    {
        if (!findRoute(e.path.str()))
            fn(e);
    });

    // Each route: only what it owns (and only if the longest match is it).
    for (const auto& r : routes)
    {
        r.source->enumerate([&](const SAssetSourceEntry& e)
        {
            if (findRoute(e.path.str()) == &r)
                fn(e);
        });
    }
}

bool MRoutedAssetSource::readBytes(const SString& path, std::vector<uint8_t>& out) const
{
    return sourceFor(path).readBytes(path, out);
}

bool MRoutedAssetSource::readText(const SString& path, SString& out) const
{
    return sourceFor(path).readText(path, out);
}

std::unique_ptr<IAssetStream> MRoutedAssetSource::openStream(const SString& path) const
{
    return sourceFor(path).openStream(path);
}

bool MRoutedAssetSource::hasMeta(const SString& path) const
{
    return sourceFor(path).hasMeta(path);
}

bool MRoutedAssetSource::readMeta(const SString& path, pugi::xml_document& out) const
{
    return sourceFor(path).readMeta(path, out);
}

SString MRoutedAssetSource::getDiskPath(const SString& path) const
{
    return sourceFor(path).getDiskPath(path);
}

// ---------------------------------------------------------------------------
// Writing
// ---------------------------------------------------------------------------

IWritableAssetSource* MRoutedAssetSource::writableFor(const SString& path, const char* op) const
{
    auto* w = sourceFor(path).asWritable();
    if (!w)
        MERROR(SString::format("RoutedAssetSource:: {0} refused, source for {1} is read-only",
                               SString(op), path));
    return w;
}

bool MRoutedAssetSource::writeBytes(const SString& path, const void* data, size_t size)
{
    auto* w = writableFor(path, "write");
    return w && w->writeBytes(path, data, size);
}

bool MRoutedAssetSource::writeMeta(const SString& path, const pugi::xml_document& doc)
{
    auto* w = writableFor(path, "writeMeta");
    return w && w->writeMeta(path, doc);
}

bool MRoutedAssetSource::remove(const SString& path)
{
    auto* w = writableFor(path, "remove");
    return w && w->remove(path);
}

bool MRoutedAssetSource::createDirectory(const SString& path)
{
    auto* w = writableFor(path, "createDirectory");
    return w && w->createDirectory(path);
}

bool MRoutedAssetSource::removeDirectory(const SString& path)
{
    auto* w = writableFor(path, "removeDirectory");
    return w && w->removeDirectory(path);
}
