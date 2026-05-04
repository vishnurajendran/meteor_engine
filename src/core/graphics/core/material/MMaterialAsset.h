//
// Created by ssj5v on 05-10-2024.
//

#ifndef MMATERIALASSET_H
#define MMATERIALASSET_H
#include <map>

#include "core/engine/assetmanagement/asset/asset.h"
#include "core/engine/assetmanagement/asset/defferedloadableasset.h"
#include "core/graphics/core/shader/shader.h"
#include "data/serialized_class_base.h"
#include "material.h"

class MMaterial;

class MMaterialAsset : public MAsset, public SerializedClassBase
{
public:
    bool hasDeferredLoad() const override { return true; }
    void deferredAssetLoad(bool forced) override;
    bool requestReload() override { loadFromFile(path); deferredAssetLoad(true); return valid; }

    DECLARE_FIELD(shaderPathField,    std::string, "")
    DECLARE_FIELD(shadingModeStr,     std::string, "lit")  // "lit" | "unlit"

public:
    explicit MMaterialAsset(const SString& path);
    ~MMaterialAsset() override;

    // Returns the shared material directly — no cloning.
    MMaterial* getMaterial();
    // Write current material properties back to the source .material file.
    bool save();

    void buildMaterialAsset();

    MMaterial::ShadingMode getShadingMode() const { return shadingMode; }
    SString                getShaderPath()  const { return SString(shaderPathField.get().c_str()); }

    // Returns true on success. The asset manager's delta refresh will pick it
    // up automatically within 5 seconds, or call refresh() to load immediately.
    static bool createNewMaterial(const SString& directory,
                                  const SString& materialName,
                                  const SString& shaderPath,
                                  MMaterial::ShadingMode mode = MMaterial::ShadingMode::Lit);

private:
    MMaterial* original   = nullptr;
    std::map<SString, SShaderPropertyValue> properties;
    MMaterial::ShadingMode shadingMode = MMaterial::ShadingMode::Lit;

    bool loadFromFile(const SString& path);
    void syncFromFields();  // field strings -> enum + cached SString
};

#endif // MMATERIALASSET_H