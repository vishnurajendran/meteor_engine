//
// Created by ssj5v on 05-10-2024.
//

#ifndef MMATERIALASSET_H
#define MMATERIALASSET_H
#include <map>

#include "core/engine/3d/shader/shader.h"
#include "core/engine/assetmanagement/asset/asset.h"
#include "core/engine/assetmanagement/asset/defferedloadableasset.h"

class MMaterial;

class MMaterialAsset : public MAsset, public IDefferedLoadableAsset{
public:
    void defferedAssetLoad(bool forced) override;
private:
    MMaterial* original = nullptr;
    SString shaderPath="";
    std::map<SString, SShaderPropertyValue> properties = {};
private:
    bool loadFromFile(const SString& path);
public:
    explicit MMaterialAsset(const SString& path);
    ~MMaterialAsset();
    MMaterial* getInstance();
    void buildMaterialAsset();
};


#endif //MMATERIALASSET_H
