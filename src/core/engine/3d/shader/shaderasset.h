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
    static const SString SHDR_ATTRIB_BASE_SOURCE;
    static const SString SHDR_PROPERTY_PARENT_NODE;
    static const SString SHDR_PROPERTY_CHILD_NODE;
    static const SString SHDR_PROPERTY_CHILD_ATTRIB_KEY;
    static const SString SHDR_PROPERTY_CHILD_ATTRIB_TYPE;
    static const SString SHDR_PROPERTY_CHILD_ATTRIB_VALUE;
    static const SString SHDR_VERT_PROGRAM_BEGIN;
    static const SString SHDR_VERT_PROGRAM_END;
    static const SString SHDR_FRAG_PROGRAM_BEGIN;
    static const SString SHDR_FRAG_PROGRAM_END;
    static const SString SHDR_CDATA_BEGIN;
    static const SString SHDR_CDATA_END;
    static const SString SHDR_VERTNODE;
    static const SString SHDR_FRAGNODE;

    static const SString SHDR_EXTENSIONS;
    static const SString SHDR_DEFINE_COMPILE_VERT;
    static const SString SHDR_DEFINE_COMPILE_FRAG;

    static const SString SHDR_FALLBACK_MISSING_VERT_PASS;
    static const SString SHDR_FALLBACK_MISSING_FRAG_PASS;

    MShader* shader{};
private:
    void loadShader(const SString& path);
    bool loadAsSubShader(const pugi::xml_node& rootNode, const SString& baseSource, const bool& hasVertPass, const bool& hasFragPass);
    bool loadAsIndependantShader(const pugi::xml_node& rootNode, const bool& hasVertPass, const bool& hasFragPass);

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
