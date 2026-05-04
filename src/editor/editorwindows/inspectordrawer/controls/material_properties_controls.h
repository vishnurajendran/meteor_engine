//
// Created by ssj5v on 16-05-2025.
//

#ifndef MATERIAL_PROPERTIES_CONTROLS_H
#define MATERIAL_PROPERTIES_CONTROLS_H
#include "core/engine/assetmanagement/asset/asset_handle.h"
#include "core/graphics/core/shader/shader.h"
#include "core/object/object.h"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"


class MAsset;
class MAssetReferenceControl;
class MMaterial;
class MMaterialAsset;

class MMaterialPropertyControl : MObject {
public:
    // Takes MMaterialAsset* and resolves getMaterial() fresh each call —
    // never caches the raw MMaterial* which can become dangling.
    void draw(MMaterialAsset* asset);
    static bool canAcceptTextureAsset(MAsset* asset);

    // Flush stale texture reference controls — called by MAssetInspector on hot reload.
    void clearTextureReferences();
    int  getLastPropertyCount() const { return lastPropertyCount; }
    void setLastPropertyCount(int n)  { lastPropertyCount = n; }

private:
    std::map<SString, MAssetReferenceControl*> textureReferences;
    int lastPropertyCount = -1;
    void drawProperty(const SString& label, SShaderPropertyValue& propertyValue, MMaterial* target);
    static void drawFloatParameter(SString label, SShaderPropertyValue& value, MMaterial* target);
    static void drawIntParameter(SString label, SShaderPropertyValue& value, MMaterial* target);
    static void drawVec2Parameter(const SString& label, SShaderPropertyValue& value, MMaterial* target);
    static void drawVec3Parameter(const SString& label, SShaderPropertyValue& value, MMaterial* target);
    static void drawVec4Parameter(const SString& label, SShaderPropertyValue& value, MMaterial* target);
    static void drawColorParameter(const SString& label, SShaderPropertyValue& value, MMaterial* target);
    void drawTextureParameter(const SString& label, SShaderPropertyValue& value, MMaterial* target);
};



#endif //MATERIAL_PROPERTIES_CONTROLS_H