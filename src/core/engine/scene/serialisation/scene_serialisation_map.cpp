#include "scene_serialisation_map.h"
#include "sceneentitytypedeserializer.h"

std::unordered_map<std::string, MSceneEntityTypeSerialiser*>&
MSceneSerialiserMap::registry()
{
    static std::unordered_map<std::string, MSceneEntityTypeSerialiser*> s;
    return s;
}

void MSceneSerialiserMap::registerSerialiser(const std::string& typeName,
                                             MSceneEntityTypeSerialiser* s)
{
    registry()[typeName] = s;
}

MSceneEntityTypeSerialiser*
MSceneSerialiserMap::getSerialiser(const std::string& typeName)
{
    auto it = registry().find(typeName);
    return (it != registry().end()) ? it->second : nullptr;
}