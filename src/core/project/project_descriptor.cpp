//
// project_descriptor.cpp
//

#include "project_descriptor.h"

#include "core/utils/logger.h"
#include "pugixml.hpp"

bool SProjectDescriptor::load(const std::filesystem::path& file)
{
    pugi::xml_document doc;
    const auto res = doc.load_file(file.c_str());
    if (!res)
    {
        MERROR(SString::format("ProjectDescriptor:: cannot read {0}: {1}",
                               SString(file.generic_string()), SString(res.description())));
        return false;
    }

    const auto root = doc.child("project");
    if (!root)
    {
        MERROR(SString("ProjectDescriptor:: missing <project> root in ") + SString(file.generic_string()));
        return false;
    }

    formatVersion = root.attribute("formatVersion").as_int(CURRENT_FORMAT_VERSION);
    name          = root.attribute("name").as_string(file.stem().string().c_str());
    id            = root.attribute("id").as_string();
    engineVersion = root.attribute("engineVersion").as_string();

    if (const auto folders = root.child("assetFolders"))
    {
        std::vector<SString> loaded;
        for (auto f : folders.children("folder"))
        {
            SString path = f.attribute("path").as_string();
            if (path.empty()) continue;
            if (path.str().back() != '/') path += "/";
            loaded.push_back(path);
        }
        if (!loaded.empty())
            assetFolders = std::move(loaded);
    }

    startupScene = root.child("startupScene").attribute("path").as_string();

    if (const auto publish = root.child("publish"))
    {
        version                = publish.attribute("version").as_string(version.c_str());
        publisher              = publish.attribute("publisher").as_string();
        publishOutputDirectory = publish.attribute("outputDirectory").as_string();
        maxPakSizeMB           = publish.attribute("maxPakSizeMB").as_int(maxPakSizeMB);
    }

    if (formatVersion > CURRENT_FORMAT_VERSION)
        MWARN(SString::format("ProjectDescriptor:: {0} uses format {1}, newer than this editor ({2})",
                              SString(file.filename().string()), SString::fromInt(formatVersion),
                              SString::fromInt(CURRENT_FORMAT_VERSION)));
    return true;
}

bool SProjectDescriptor::save(const std::filesystem::path& file) const
{
    pugi::xml_document doc;
    auto root = doc.append_child("project");
    root.append_attribute("formatVersion").set_value(formatVersion);
    root.append_attribute("name").set_value(name.c_str());
    root.append_attribute("id").set_value(id.c_str());
    root.append_attribute("engineVersion").set_value(engineVersion.c_str());

    auto folders = root.append_child("assetFolders");
    for (const auto& f : assetFolders)
        folders.append_child("folder").append_attribute("path").set_value(f.c_str());

    if (!startupScene.empty())
        root.append_child("startupScene").append_attribute("path").set_value(startupScene.c_str());

    auto publish = root.append_child("publish");
    publish.append_attribute("version").set_value(version.c_str());
    publish.append_attribute("publisher").set_value(publisher.c_str());
    publish.append_attribute("outputDirectory").set_value(publishOutputDirectory.c_str());
    publish.append_attribute("maxPakSizeMB").set_value(maxPakSizeMB);

    if (!doc.save_file(file.c_str()))
    {
        MERROR(SString("ProjectDescriptor:: cannot write ") + SString(file.generic_string()));
        return false;
    }
    return true;
}

SProjectDescriptor SProjectDescriptor::makeDefault(const std::filesystem::path& projectRoot)
{
    SProjectDescriptor d;
    d.name = SString(projectRoot.filename().string());
    return d;
}
