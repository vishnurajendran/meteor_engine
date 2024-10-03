//
// Created by Vishnu Rajendran on 2024-09-28.
//

#ifndef METEOR_ENGINE_SPATIALENTITYINSPECTORDRAWER_H
#define METEOR_ENGINE_SPATIALENTITYINSPECTORDRAWER_H

#include "inspectordrawer.h"
#include "imgui.h"
#include "../../../../cmake-build-player_debug/_deps/sfml-src/include/SFML/Graphics/Texture.hpp"
#include "core/utils/glmhelper.h"

class MSpatialEntityInspectorDrawer : public MInspectorDrawer {
private:
    sf::Texture trfTexture;
    sf::Vector2f trfSize;
public:
    MSpatialEntityInspectorDrawer();
    void onDrawInspector(MSpatialEntity *target) override;

protected:
    void drawColoredBoxOverLabel(const char* label, ImVec4 boxColor, float boxWidth);
    bool drawXYZComponent(const SString& label, SVector3& value);
private:
    bool drawTextField(const SString& label, SString& text);
    void drawTransformField(MSpatialEntity* target);
};

#endif //METEOR_ENGINE_SPATIALENTITYINSPECTORDRAWER_H
