#pragma once
#ifndef SCENEENTITYTYPEDESERIALIZER_H
#define SCENEENTITYTYPEDESERIALIZER_H

#include "core/utils/color.h"
#include "core/utils/glmhelper.h"
#include "core/utils/sstring.h"
#include "pugixml.hpp"

class MSpatialEntity;

class MSceneEntityTypeDeserializer
{
public:
    virtual ~MSceneEntityTypeDeserializer() = default;

    // "attrib" matches the actual .scml file format (<attrib> node)
    static inline const SString ATTRIB_NODE      = "attrib";
    static inline const SString ATTRIB_VALUE_KEY = "value";

    virtual MSpatialEntity* deserialize(pugi::xml_node node) = 0;
    virtual pugi::xml_node  serialise(MSpatialEntity* entity,
                                      pugi::xml_node  parent) = 0;

    // ── Helpers ──────────────────────────────────────────────────────────────
    // Reads name attribute + attrib/transform block.
    void parseSpatialData(pugi::xml_node node, MSpatialEntity* entity);

    static void parseVector3(const char* str, SVector3& out);
    static void parseVector4(const char* str, SVector4& out);

    // writeSpatialBase creates:
    //   <typeName name="entityName">
    //     <attrib>
    //       <transform>
    //         <localPosition value="(x,y,z)"/>
    //         <localRotation value="(x,y,z,w)"/>
    //         <localScale    value="(x,y,z)"/>
    //       </transform>
    //     </attrib>
    //   </typeName>
    // Returns the entity node. The <attrib> child is accessible via
    // entityNode.child(ATTRIB_NODE.c_str()) for type-specific data.
    static pugi::xml_node writeSpatialBase(MSpatialEntity* entity,
                                           pugi::xml_node  parent,
                                           const SString&  typeName);

    static void writeFloat (pugi::xml_node p, const char* tag, float v);
    static void writeInt   (pugi::xml_node p, const char* tag, int   v);
    static void writeBool  (pugi::xml_node p, const char* tag, bool  v);
    static void writeString(pugi::xml_node p, const char* tag, const SString& v);
    static void writeVec3  (pugi::xml_node p, const char* tag, const SVector3& v);
    static void writeVec4  (pugi::xml_node p, const char* tag, const SVector4& v);
    static void writeColor (pugi::xml_node p, const char* tag, const SColor&   v);
};

#endif