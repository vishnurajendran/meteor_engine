//
// Created by ssj5v on 03-10-2024.
//
#pragma once
#ifndef SHADERASSET_H
#define SHADERASSET_H
#include "shader.h"
#include "core/engine/assetmanagement/asset/asset.h"

class MShader;

class MShaderAsset : public MAsset {
private:
    MShader* shader;
private:
    void loadShader(const SString& path);
public:
    explicit MShaderAsset(const SString &path);
    ~MShaderAsset() override;
    [[nodiscard]] MShader* getShader() const;

    static SShaderPropertyType parsePropertyType(const SString& str);
    static void parseValue(const SString& str, SShaderPropertyValue& value, const SShaderPropertyType& type);
};



#endif //SHADERASSET_H
