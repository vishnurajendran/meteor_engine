//
// Created by ssj5v on 03-10-2024.
//
#pragma once
#ifndef SHADERASSET_H
#define SHADERASSET_H
#include <vector>
#include "shader.h"
#include "core/engine/assetmanagement/asset/asset.h"

namespace pugi {
    class xml_node;
}

class MShaderBase;
class MStandardShader;
class MComputeShader;

SCRIPT_BIND_CLASS()
class MShaderAsset : public MAsset {
    DEFINE_OBJECT_SUBCLASS(MShaderAsset)
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
    static const SString SHDR_COMP_PROGRAM_BEGIN;
    static const SString SHDR_COMP_PROGRAM_END;
    static const SString SHDR_CDATA_BEGIN;
    static const SString SHDR_CDATA_END;
    static const SString SHDR_VERTNODE;
    static const SString SHDR_FRAGNODE;
    static const SString SHDR_COMPNODE;

    static const SString SHDR_EXTENSIONS;
    static const SString SHDR_DEFINE_COMPILE_VERT;
    static const SString SHDR_DEFINE_COMPILE_FRAG;
    static const SString SHDR_DEFINE_COMPILE_COMP;

    static const SString SHDR_FALLBACK_MISSING_VERT_PASS;
    static const SString SHDR_FALLBACK_MISSING_FRAG_PASS;

    MShaderBase* shader{};

    // Shaders replaced by a hot reload. Materials (and anything else) may
    // still hold raw shader pointers to them, so they are only freed when the asset
    // itself is destroyed. Once reloads notify dependent materials, these can
    // be deleted right after the swap instead.
    std::vector<MShaderBase*> retiredShaders;
private:
    // Reads the shader (and its base source, if any) through the active asset
    // source. On success swaps in the new shader; on failure keeps the last
    // good one so a typo during hot reload doesn't break every material.
    void loadShader();
    MStandardShader* loadAsSubShader(const pugi::xml_node& rootNode, const SString& baseSource, const bool& hasVertPass, const bool& hasFragPass);
    MStandardShader* loadAsIndependantShader(const pugi::xml_node& rootNode, const bool& hasVertPass, const bool& hasFragPass);
    // A .mesl with a #COMPUTE_PASS is a compute shader: no vertex or fragment
    // pass, no base source.
    MComputeShader* loadAsComputeShader(const pugi::xml_node& rootNode);

    [[nodiscard]] static std::unordered_map<SString, SShaderPropertyValue> getShaderProperties(
        pugi::xml_node node, std::vector<SString>& outOrder);
    [[nodiscard]] static std::pair<SString, SString> getShaderNameAndVersion(pugi::xml_node node);

public:
    explicit MShaderAsset(const SString &path);
    ~MShaderAsset() override;
    // The draw shader, or nullptr if this .mesl is a compute shader.
    [[nodiscard]] MStandardShader* getShader() const;
    // The compute shader, or nullptr if this .mesl is a draw shader.
    [[nodiscard]] MComputeShader* getComputeShader() const;
    // Whichever kind it is.
    [[nodiscard]] MShaderBase* getShaderBase() const { return shader; }
    // From the last good load. Decides the asset browser icon too.
    [[nodiscard]] bool isComputeShader() const;

    bool requestReload() override { loadShader(); return valid; }
};



#endif //SHADERASSET_H