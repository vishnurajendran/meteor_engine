#pragma once
#ifndef SCENE_SERIALISER_MAP_H
#define SCENE_SERIALISER_MAP_H

#include <unordered_map>
#include <string>

class MSceneEntityTypeSerialiser;

// Registry: type-name → serialiser instance.
// Parallel to MSceneEntityTypeMap but for writing.
class MSceneSerialiserMap
{
public:
    static void registerSerialiser(const std::string& typeName,
                                   MSceneEntityTypeSerialiser* serialiser);

    static MSceneEntityTypeSerialiser* getSerialiser(const std::string& typeName);

private:
    static std::unordered_map<std::string, MSceneEntityTypeSerialiser*>& registry();
};

#endif //SCENE_SERIALISER_MAP_H