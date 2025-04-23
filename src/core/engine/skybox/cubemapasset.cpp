//
// Created by ssj5v on 29-03-2025.
//

#include "cubemapasset.h"
#include "core/utils/fileio.h"
#include "core/utils/logger.h"
#include "cubemaptexture.h"
#include "pugixml.hpp"

MCubemapAsset::MCubemapAsset(const SString& path)
{
    this->path = path;
    SString data;
    valid = false;
    if (FileIO::readFile(path,data))
    {
        pugi::xml_document doc;
        doc.load_string(data.c_str());
        pugi::xml_node root = doc.child("cubemap");
        name = root.attribute("name").as_string();
        std::vector<SString> paths;
        std::vector<SString> sideLabels = {
            "right",
            "left",
            "top",
            "bottom",
            "back",
            "front"
        };

        for (const auto& label : sideLabels)
        {
            paths.emplace_back(root.child(label.c_str()).attribute("src").as_string());
        }
        texture = MCubemapTexture::createCubeMap(paths);
        valid = true;
    }
    else
    {
        MERROR(STR("MTextureAsset::MCubemapAsset could not load file ") + path);
    }
}


MTexture* MCubemapAsset::getTexture()
{
    return texture;
}
