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
    static void DrawTextureRect(const SVector3& position, const SVector2& halfExtents, MTexture* texture);
    static void DrawWireCube(SVector3 position, SVector3 halfExtents, SColor color);
    static void DrawWireSphere(SVector3 position, float radius, SColor color);
private:
    static MCameraEntity* getActiveCamera();
    static SVector2 getResolution();
public:

    static void requestGizmoDraws();
    static void recursiveGizmoDraws(MSpatialEntity* entity);
};

#endif //GIZMOS_H
