//
// Created by ssj5v on 12-05-2025.
//

#ifndef OBJECT_REFERENCE_CONTROLS_H
#define OBJECT_REFERENCE_CONTROLS_H
#include <functional>


#include "SFML/Graphics/Texture.hpp"
#include "core/engine/assetmanagement/asset/asset_handle.h"
#include "core/object/object.h"


class MAssetManager;
class MAsset;
class MAssetReferenceControl : public MObject {
private:
    SString assetIdReference = "";
private:
    sf::Texture* getFileIcon(MAssetManager* assetManager, TAssetHandle<MAsset> asset) const;
    static bool defaultTestFuncCallback(TAssetHandle<MAsset> asset);
public:
    static const SString ASSET_REF_TARGET_KEY;
    MAssetReferenceControl();
    explicit MAssetReferenceControl(TAssetHandle<MAsset> asset);
    MAsset* getAssetReference() const;
    void setAssetReference(MAsset* asset);
    bool drawControl(const SString& label);

    // Single-row compact version — fits inside inspector table cells.
    // Renders: [32px thumb] [asset name or "(none)"] [× clear]
    bool drawCompactControl(const SString& label);
    std::function<bool(MAsset* asset)> canAcceptAssetFuncCallback;
};

#endif //OBJECT_REFERENCE_CONTROLS_H