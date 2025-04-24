//
// Created by ssj5v on 22-04-2025.
//

#ifndef GIZMOS_H
#define GIZMOS_H
#include "core/object/object.h"
#include "core/utils/color.h"
#include "core/utils/glmhelper.h"


class MSpatialEntity;
class MCameraEntity;
class MTexture;
class MGizmos : MObject {
public:
    static void drawTextureRect(const SVector3& position, const SVector2& halfExtents, MTexture* texture);
    static void drawWireCube(SVector3 position, SVector3 halfExtents, SColor color, float thickness);
    static void drawWireSphere(SVector3 position, float radius, SColor color, float thickness);
    static void drawRay(const SVector3& origin, const SVector3& direction, const float& length, const SColor& color, const float& thickness);
    static void drawLine(const SVector3& start, const SVector3& end, const SColor& color, const float& thickness, bool ignoreZDepth);
    static void drawWireFrustum(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, SColor color, float thickness);
private:
    static MCameraEntity* getActiveCamera();
    static SVector2 getResolution();

    static unsigned int uiQuadVAO;

    static unsigned int uiLineVAO;
    static unsigned int uiLineVBO;
private:
    static unsigned int createQuadVAO();
    static void createLineVAO();

public:

    static void requestGizmoDraws();
    static void recursiveGizmoDraws(MSpatialEntity* entity);
};

#endif //GIZMOS_H
