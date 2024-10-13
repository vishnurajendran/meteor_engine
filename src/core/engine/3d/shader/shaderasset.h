//
// Created by ssj5v on 03-10-2024.
//
#pragma once
#ifndef SHADERASSET_H
#define SHADERASSET_H
#include "shader.h"
#include "core/engine/assetmanagement/asset/asset.h"

namespace pugi {
    class xml_node;
}

class MShader;

class MShaderAsset : public MAsset {
private:
    static const SString SHDR_ROOTNODE;
    static const SString SHDR_VERTNODE;
    static const SString SHDR_FRAGNODE;
    static const SString SHDR_INCLNODE;
    static const SString SHDR_INCL_ATTRIB_PATH;
    static const SString SHDR_INCL_ATTRIB_INTERNAL;
    static const SString SHDR_PROPERTY_PARENT_NODE;
    static const SString SHDR_PROPERTY_CHILD_NODE;
    static const SString SHDR_PROPERTY_CHILD_ATTRIB_KEY;
    static const SString SHDR_PROPERTY_CHILD_ATTRIB_TYPE;
    static const SString SHDR_PROPERTY_CHILD_ATTRIB_VALUE;

    MShader* shader{};
private:
    void loadShader(const SString& path);

    [[nodiscard]] static SString getPrequisiteCode(pugi::xml_node node);
    [[nodiscard]] static std::map<SString, SShaderPropertyValue> getShaderProperties(pugi::xml_node node);
    [[nodiscard]] static std::pair<SString, SString> getShaderNameAndVersion(pugi::xml_node node);

public:
    explicit MShaderAsset(const SString &path);
    ~MShaderAsset() override;
    [[nodiscard]] MShader* getShader() const;

    static SShaderPropertyType parsePropertyType(const SString& str);
    static void parseValue(const SString& str, SShaderPropertyValue& value, const SShaderPropertyType& type);
};



#endif //SHADERASSET_H
