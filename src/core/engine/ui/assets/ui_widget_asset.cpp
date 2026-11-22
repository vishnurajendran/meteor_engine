//
// ui_widget_asset.cpp
//

#include "ui_widget_asset.h"

#include <sstream>

#include "core/engine/assetmanagement/assetmanager/asset_manager_subsystem.h"
#include "core/engine/assetmanagement/source/asset_sources.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/utils/fileio.h"
#include "core/utils/logger.h"
#include "pugixml.hpp"

namespace
{
    IAssetManagerSubsystem* assetManager()
    {
        return MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>();
    }

    // Resolved path of a reference, or empty.
    SString resolvedPath(const SString& reference)
    {
        auto* manager = assetManager();
        if (!manager || reference.empty())
            return {};
        const auto handle = manager->getAssetFromReference<MAsset>(reference);
        const MAsset* asset = handle.get();
        return asset ? asset->getPath() : SString();
    }
}

MUIWidgetAsset::MUIWidgetAsset(const SString& path) : MAsset(path)
{
    name  = FileIO::getFileName(path);
    valid = load();
}

SString MUIWidgetAsset::getWidgetName() const
{
    // An identifier, so it is usable as a CSS class: "My Hud" -> "My_Hud".
    std::string stem = FileIO::getFileNameWithoutExtension(path).str();
    for (char& c : stem)
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-')
            c = '_';
    return SString(stem.empty() ? std::string("Widget") : stem);
}

TAssetHandle<MUIStructureAsset> MUIWidgetAsset::getStructure() const
{
    auto* manager = assetManager();
    return manager ? manager->getAssetFromReference<MUIStructureAsset>(structureRef) : TAssetHandle<MUIStructureAsset>();
}

std::vector<TAssetHandle<MUIStyleAsset>> MUIWidgetAsset::getStyles() const
{
    std::vector<TAssetHandle<MUIStyleAsset>> styles;
    auto* manager = assetManager();
    if (!manager)
        return styles;
    for (const auto& reference : styleRefs)
        styles.push_back(manager->getAssetFromReference<MUIStyleAsset>(reference));
    return styles;
}

TAssetHandle<MAsset> MUIWidgetAsset::getBehaviour() const
{
    auto* manager = assetManager();
    if (!manager || behaviourRef.empty())
        return {};
    return manager->getAssetFromReference<MAsset>(behaviourRef);
}

bool MUIWidgetAsset::requestReload()
{
    valid = load();
    return valid;
}

bool MUIWidgetAsset::load()
{
    SString text;
    if (!MAssetSources::getActive()->readText(path, text))
    {
        MERROR("MUIWidgetAsset: failed to read " + path);
        return false;
    }

    pugi::xml_document doc;
    const auto result = doc.load_string(text.c_str());
    const auto root = doc.child("widget");
    if (!result || !root)
    {
        MERROR(SString::format("MUIWidgetAsset: '{0}' is not a widget file ({1})", path, SString(result.description())));
        return false;
    }

    const int version = root.attribute("version").as_int(FILE_VERSION);
    if (version > FILE_VERSION)
        MWARN(SString::format("MUIWidgetAsset: '{0}' is version {1}; this engine reads version {2}",
                              path, SString::fromInt(version), SString::fromInt(FILE_VERSION)));

    tickWhenPaused = root.attribute("tickWhenPaused").as_bool(true);
    structureRef   = SString(root.child("structure").text().as_string()).trimmed();
    behaviourRef   = SString(root.child("behaviour").text().as_string()).trimmed();
    styleRefs.clear();
    for (auto style : root.children("style"))
    {
        const SString reference = SString(style.text().as_string()).trimmed();
        if (!reference.empty())
            styleRefs.push_back(reference);
    }

    if (structureRef.empty())
    {
        MERROR("MUIWidgetAsset: '" + path + "' has no <structure>");
        return false;
    }
    clearDirty();
    return true;
}

void MUIWidgetAsset::deferredAssetLoad(bool /*forced*/)
{
    if (!getStructure().isValid())
        MWARN(SString::format("UI widget '{0}': its structure '{1}' is not a .rml asset", path, structureRef));
    for (const auto& reference : styleRefs)
        if (!assetManager() || !assetManager()->getAssetFromReference<MUIStyleAsset>(reference).isValid())
            MWARN(SString::format("UI widget '{0}': its style '{1}' is not a .rcss asset", path, reference));
    if (!behaviourRef.empty() && !getBehaviour().isValid())
        MWARN(SString::format("UI widget '{0}': its behaviour '{1}' was not found", path, behaviourRef));
}

bool MUIWidgetAsset::dependsOn(const SString& assetPath) const
{
    if (resolvedPath(structureRef) == assetPath || resolvedPath(behaviourRef) == assetPath)
        return true;
    for (const auto& reference : styleRefs)
        if (resolvedPath(reference) == assetPath)
            return true;
    return false;
}

SString MUIWidgetAsset::serialise(const SString& structureRef, const std::vector<SString>& styleRefs,
                                  const SString& behaviourRef, bool tickWhenPaused)
{
    pugi::xml_document doc;
    auto declaration = doc.append_child(pugi::node_declaration);
    declaration.append_attribute("version") = "1.0";

    auto root = doc.append_child("widget");
    root.append_attribute("version")        = FILE_VERSION;
    root.append_attribute("tickWhenPaused") = tickWhenPaused;
    root.append_child("structure").text().set(structureRef.c_str());
    for (const auto& reference : styleRefs)
        root.append_child("style").text().set(reference.c_str());
    if (!behaviourRef.empty())
        root.append_child("behaviour").text().set(behaviourRef.c_str());

    std::ostringstream out;
    doc.save(out, "    ");
    return SString(out.str());
}

bool MUIWidgetAsset::save()
{
    auto target = MAssetSources::getWritable();
    if (!target)
    {
        MERROR("MUIWidgetAsset::save - active asset source is read-only: " + path);
        return false;
    }

    // References are always written as GUIDs.
    auto canonical = [](const SString& reference) -> SString
    {
        auto* manager = assetManager();
        return manager && !reference.empty() ? manager->toCanonicalReference<MAsset>(reference) : reference;
    };
    std::vector<SString> styles;
    for (const auto& reference : styleRefs)
        styles.push_back(canonical(reference));

    const SString text = serialise(canonical(structureRef), styles, canonical(behaviourRef), tickWhenPaused);
    if (!target->writeText(path, text))
    {
        MERROR("MUIWidgetAsset::save - failed to write " + path);
        return false;
    }
    structureRef = canonical(structureRef);
    behaviourRef = canonical(behaviourRef);
    styleRefs    = styles;
    clearDirty();
    return true;
}
