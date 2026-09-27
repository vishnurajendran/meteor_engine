//
// Created by ssj5v on 05-10-2024.
//

#ifndef MMATERIALASSET_H
#define MMATERIALASSET_H
#include <map>
#include <vector>

#include "core/engine/assetmanagement/asset/asset.h"
#include "core/engine/assetmanagement/asset/defferedloadableasset.h"
#include "core/graphics/core/shader/shader.h"
#include "data/serialized_class_base.h"
#include "material.h"

class MMaterial;

class MMaterialAsset : public MAsset, public SerializedClassBase
{
    DEFINE_OBJECT_SUBCLASS(MMaterialAsset)
public:
    bool hasDeferredLoad() const override { return true; }
    void deferredAssetLoad(bool forced) override;
    bool dependsOn(const SString& assetPath) const override { return getShaderPath() == assetPath; }
    bool requestReload() override { valid = loadFromSource(); deferredAssetLoad(true); return valid; }

    DECLARE_FIELD(shaderPathField,    std::string, "")
    DECLARE_FIELD(shadingModeStr,     std::string, "lit")  // "lit" | "unlit"

public:
    explicit MMaterialAsset(const SString& path);
    ~MMaterialAsset() override;

    MMaterial* getMaterial();
    bool save() override;

    void buildMaterialAsset();

    MMaterial::ShadingMode getShadingMode() const { return shadingMode; }
    SString                getShaderPath()  const { return SString(shaderPathField.get().c_str()); }

    // New materials are created by the editor's "material" asset template
    // (builtin_asset_templates.cpp), via
    // MEditorAssetManager::createAssetFromTemplate().

private:
    MMaterial* original   = nullptr;
    std::map<SString, SShaderPropertyValue> properties;
    std::vector<SString> loadOrder;  // property key order from the XML file
    MMaterial::ShadingMode shadingMode = MMaterial::ShadingMode::Lit;

    bool loadFromSource();
    void syncFromFields();
};

#endif // MMATERIALASSET_H