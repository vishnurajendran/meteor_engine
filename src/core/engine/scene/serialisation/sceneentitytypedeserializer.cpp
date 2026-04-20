#include "sceneentitytypedeserializer.h"
#include "core/engine/entities/spatial/spatial.h"
#include <cstdio>

// ── parseSpatialData ─────────────────────────────────────────────────────────
// Name lives as an XML attribute on the entity node itself.
// Transform lives in <attrib><transform> with localPosition/localRotation/localScale.
// Values are formatted as (x,y,z) or (x,y,z,w).
// Rotation is stored as a quaternion (x,y,z,w).

void MSceneEntityTypeDeserializer::parseSpatialData(pugi::xml_node node,
                                                     MSpatialEntity* entity)
{
    // Name — XML attribute on the entity node: <static_mesh name="Cube">
    if (const char* n = node.attribute("name").value())
        if (n[0] != '\0') entity->setName(n);

    const auto attrib = node.child(ATTRIB_NODE.c_str());
    if (!attrib) return;

    const auto transform = attrib.child("transform");
    if (!transform) return;

    SVector3 pos   = {};
    SVector3 scale = { 1, 1, 1 };
    SVector4 rot   = { 0, 0, 0, 1 }; // x,y,z,w — identity

    if (const auto n = transform.child("localPosition"))
        parseVector3(n.attribute(ATTRIB_VALUE_KEY.c_str()).value(), pos);
    if (const auto n = transform.child("localRotation"))
        parseVector4(n.attribute(ATTRIB_VALUE_KEY.c_str()).value(), rot);
    if (const auto n = transform.child("localScale"))
        parseVector3(n.attribute(ATTRIB_VALUE_KEY.c_str()).value(), scale);

    entity->setRelativePosition(pos);
    // File stores quaternion as (x,y,z,w); GLM quat constructor is (w,x,y,z)
    entity->setRelativeRotation(SQuaternion(rot.w, rot.x, rot.y, rot.z));
    entity->setRelativeScale(scale);
}

void MSceneEntityTypeDeserializer::parseVector3(const char* str, SVector3& out)
{
    if (!str || str[0] == '\0') return;
    sscanf(str, "(%f,%f,%f)", &out.x, &out.y, &out.z);
}

void MSceneEntityTypeDeserializer::parseVector4(const char* str, SVector4& out)
{
    if (!str || str[0] == '\0') return;
    sscanf(str, "(%f,%f,%f,%f)", &out.x, &out.y, &out.z, &out.w);
}

// ── writeSpatialBase ─────────────────────────────────────────────────────────

pugi::xml_node MSceneEntityTypeDeserializer::writeSpatialBase(
    MSpatialEntity* entity, pugi::xml_node parent, const SString& typeName)
{
    pugi::xml_node node = parent.append_child(typeName.c_str());
    node.append_attribute("name") = entity->getName().c_str();

    pugi::xml_node attrib    = node.append_child(ATTRIB_NODE.c_str());
    pugi::xml_node transform = attrib.append_child("transform");

    const SVector3    pos   = entity->getRelativePosition();
    const SQuaternion q     = entity->getRelativeRotation();
    const SVector3    scale = entity->getRelativeScale();

    // Write position/scale as (x,y,z)
    writeVec3(transform, "localPosition", pos);
    // Rotation as (x,y,z,w) — GLM quat stores w first internally
    writeVec4(transform, "localRotation", SVector4(q.x, q.y, q.z, q.w));
    writeVec3(transform, "localScale",    scale);

    return node; // caller accesses attrib via node.child(ATTRIB_NODE)
}

// ── Value writers — all use (x,y,z) / (x,y,z,w) format ─────────────────────

void MSceneEntityTypeDeserializer::writeFloat(pugi::xml_node p, const char* tag, float v)
{
    p.append_child(tag).append_attribute(ATTRIB_VALUE_KEY.c_str()) = v;
}

void MSceneEntityTypeDeserializer::writeInt(pugi::xml_node p, const char* tag, int v)
{
    p.append_child(tag).append_attribute(ATTRIB_VALUE_KEY.c_str()) = v;
}

void MSceneEntityTypeDeserializer::writeBool(pugi::xml_node p, const char* tag, bool v)
{
    p.append_child(tag).append_attribute(ATTRIB_VALUE_KEY.c_str()) = (v ? 1 : 0);
}

void MSceneEntityTypeDeserializer::writeString(pugi::xml_node p, const char* tag,
                                                const SString& v)
{
    p.append_child(tag).append_attribute(ATTRIB_VALUE_KEY.c_str()) = v.c_str();
}

void MSceneEntityTypeDeserializer::writeVec3(pugi::xml_node p, const char* tag,
                                              const SVector3& v)
{
    char buf[128];
    snprintf(buf, sizeof(buf), "(%f,%f,%f)", v.x, v.y, v.z);
    p.append_child(tag).append_attribute(ATTRIB_VALUE_KEY.c_str()) = buf;
}

void MSceneEntityTypeDeserializer::writeVec4(pugi::xml_node p, const char* tag,
                                              const SVector4& v)
{
    char buf[160];
    snprintf(buf, sizeof(buf), "(%f,%f,%f,%f)", v.x, v.y, v.z, v.w);
    p.append_child(tag).append_attribute(ATTRIB_VALUE_KEY.c_str()) = buf;
}

void MSceneEntityTypeDeserializer::writeColor(pugi::xml_node p, const char* tag,
                                               const SColor& v)
{
    writeVec4(p, tag, SVector4(v.r, v.g, v.b, v.a));
}