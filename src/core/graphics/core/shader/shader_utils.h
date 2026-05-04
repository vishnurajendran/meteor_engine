//
// Created by ssj5v on 03-05-2026.
//

#ifndef SHADER_UTILS_H
#define SHADER_UTILS_H

#include "core/utils/serialisation_utils.h"

class MShaderUtility
{
public:
    // Helpers
    static SString getTypeStr(SShaderPropertyType type)
    {
        switch (type) {
        case Int:          return "i";
        case Float:        return "f";
        case UniformVec2:  return "u2";
        case UniformVec3:  return "u3";
        case UniformVec4:  return "u4";
        case Color:        return "col";
        case Matrix4:      return "m4";
        case Texture:      return "tex2d";
        default:           return "";
        }
    }

    static SString getValueStr(SShaderPropertyValue value)
    {
        std::ostringstream ss;
        switch (value.getType()) {
        case Int:         ss << value.getIntVal();   break;
        case Float:       ss << value.getFloatVal(); break;
        case UniformVec2: { auto u = value.getVec2Val(); ss<<"("<<u.x<<","<<u.y<<")"; break; }
        case UniformVec3: { auto u = value.getVec3Val(); ss<<"("<<u.x<<","<<u.y<<","<<u.z<<")"; break; }
        case UniformVec4: { auto u = value.getVec4Val(); ss<<"("<<u.x<<","<<u.y<<","<<u.z<<","<<u.w<<")"; break; }
        case Color:       { auto u = value.getColor();   ss<<"("<<u.r<<","<<u.g<<","<<u.b<<","<<u.a<<")"; break; }
        case Texture:     ss << value.getTexAssetReference().str(); break;
        default: break;
        }
        return ss.str();
    }

    static SShaderPropertyType parsePropertyType(const SString& str)
    {
        if (str.toLower() == "i") return Int;
        if (str.toLower() == "f") return Float;
        if (str.toLower() == "u2") return UniformVec2;
        if (str.toLower() == "u3") return UniformVec3;
        if (str.toLower() == "u4") return UniformVec4;
        if (str.toLower() == "col") return Color;
        if (str.toLower() == "m4") return Matrix4;
        if (str.toLower() == "tex2d") return Texture;
        if (str.toLower() == "tex") return Texture;
        return NoVal;
    }

    static void parseValue(const SString& str, SShaderPropertyValue& value, const SShaderPropertyType& type)
    {
        switch (type) {
        case NoVal: // no value in this property
        case Matrix4: //material serialisation not supported
            break;
        case Int:
            value.setIntVal(std::stoi(str));
            break;

        case Float:
            value.setFloatVal(std::stof(str));
            break;

        case UniformVec2: {
            SVector2 vec2 = SVector2(0);
            if (parseVector2(str, vec2)) {
                value.setVec2Val(vec2);
            }
        }
            break;

        case UniformVec3: {
            SVector3 vec3 = SVector3(0);
            if (parseVector3(str, vec3)) {
                value.setVec3Val(vec3);
            }
        }
            break;

            // Color and Vec4 are identical
        case Color: {
            SVector4 colVec4 = SVector4(0);
            if (parseVector4(str, colVec4)) {
                value.setColVal(colVec4);
            }
        }
            break;

        case UniformVec4: {
            SVector4 vec4 = SVector4(0);
            if (parseVector4(str, vec4)) {
                value.setVec4Val(vec4);
            }
        }
            break;

        case Texture: {
            value.setTextureReference(str);
        }
            break;
        }
    }
};

#endif //SHADER_UTILS_H
