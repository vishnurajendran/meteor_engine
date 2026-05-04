//
// scene_serializer.h
//
// Saves and loads an entire MScene to/from XML.
//
// Usage:
//   MSceneSerializer::save(scene, "levels/level01.scene");
//   MSceneSerializer::load("levels/level01.scene", scene);
//
// Scene file format:
//
//   <scene name="Level01">
//     <entity type="spatial" name="Floor" enabled="true">
//       <relativePosition><x>0</x><y>0</y><z>0</z></relativePosition>
//       ...
//       <children>
//         <entity type="point_light" name="LightA">
//           <intensity>2.5</intensity>
//           ...
//         </entity>
//       </children>
//     </entity>
//     <entity type="camera" name="MainCamera">
//       ...
//     </entity>
//   </scene>
//
// Only ROOT entities are written at the top level; children are nested under
// their parent's <children> block. The full hierarchy is preserved exactly.
//

#ifndef SCENE_SERIALIZER_H
#define SCENE_SERIALIZER_H

#include <string>

#include "core/engine/scene/sceneasset.h"

class MScene;

class MSceneSerializer
{
public:
    // Write `scene` to `asset`. Overwrites any existing file.
    // Returns true on success.
    static bool save(MScene* scene, const std::string& filePath);

    // Read and populate `scene` (appending to any existing entities).
    // Returns true on success.
    static bool load(const std::string& filePath, MScene* scene);

private:
    MSceneSerializer() = delete;
};

#endif // SCENE_SERIALIZER_H