//
// Created by ssj5v on 05-10-2024.
//

#pragma once
#ifndef MATERIAL_H
#define MATERIAL_H
#include <map>
#include "core/engine/3d/shader/shader.h"
#include "core/object/object.h"


class MShader;

class MMaterial : public MObject {
private:
    MShader* shader;
    std::map<SString, SShaderPropertyValue> properties;
public:
    explicit MMaterial(MShader* shader);
    ~MMaterial() override = default;
    void bindMaterial() const;
    [[nodiscard]] std::map<SString, SShaderPropertyValue> getProperties() const;
    void setProperty(const SString& name, const SShaderPropertyValue& value);
    MMaterial* clone() const;
};



#endif //MATERIAL_H
