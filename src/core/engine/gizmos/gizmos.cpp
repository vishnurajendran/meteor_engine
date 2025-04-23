//
// Created by ssj5v on 22-04-2025.
//

#include "GL/gl.h"
#include "gizmos.h"
#include "core/engine/camera/viewmanagement.h"
#include "core/engine/scene/scene.h"
#include "core/engine/scene/scenemanager.h"
#include "core/engine/texture/texture.h"
#include "core/graphics/core/graphicsrenderer.h"

MCameraEntity* MGizmos::getActiveCamera()
{
    auto& cameras = MViewManagement::getCameras();
    if (cameras.empty())
        return nullptr;
    return cameras.back(); // Highest priority (sorted in increasing order)
}
SVector2 MGizmos::getResolution()
{
    return MGraphicsRenderer::getResolution();
}


void MGizmos::DrawTextureRect(const SVector3& position, const SVector2& halfExtents, MTexture* texture)
{
    MCameraEntity* cam = getActiveCamera();
    if (!cam || !texture) return;

    SMatrix4 vp = cam->getProjectionMatrix(getResolution()) * cam->getViewMatrix(); // Replace with actual resolution
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(glm::value_ptr(vp));

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    texture->bind(0, 0);
    glEnable(GL_TEXTURE_2D);
    glColor4f(1, 1, 1, 1);

    glBegin(GL_QUADS);
    float x = position.x, y = position.y, z = position.z;
    float w = halfExtents.x, h = halfExtents.y;

    glTexCoord2f(0.f, 0.f); glVertex3f(x - w, y - h, z);
    glTexCoord2f(1.f, 0.f); glVertex3f(x + w, y - h, z);
    glTexCoord2f(1.f, 1.f); glVertex3f(x + w, y + h, z);
    glTexCoord2f(0.f, 1.f); glVertex3f(x - w, y + h, z);
    glEnd();

    glDisable(GL_TEXTURE_2D);
}

void MGizmos::DrawWireCube(SVector3 position, SVector3 halfExtents, SColor color)
{
    MCameraEntity* cam = getActiveCamera();
    if (!cam)
        return;

    SMatrix4 vp = cam->getProjectionMatrix(getResolution()) * cam->getViewMatrix();
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(glm::value_ptr(vp));
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glColor4f(color.r, color.g, color.b, color.a);
    glBegin(GL_LINES);

    SVector3 corners[8] = {
        position + SVector3(-halfExtents.x, -halfExtents.y, -halfExtents.z),
        position + SVector3(halfExtents.x, -halfExtents.y, -halfExtents.z),
        position + SVector3(halfExtents.x, halfExtents.y, -halfExtents.z),
        position + SVector3(-halfExtents.x, halfExtents.y, -halfExtents.z),
        position + SVector3(-halfExtents.x, -halfExtents.y, halfExtents.z),
        position + SVector3(halfExtents.x, -halfExtents.y, halfExtents.z),
        position + SVector3(halfExtents.x, halfExtents.y, halfExtents.z),
        position + SVector3(-halfExtents.x, halfExtents.y, halfExtents.z),
    };

    const int edges[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
                              {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};

    for (auto& edge : edges)
    {
        glVertex3fv(glm::value_ptr(corners[edge[0]]));
        glVertex3fv(glm::value_ptr(corners[edge[1]]));
    }

    glEnd();
}
void MGizmos::DrawWireSphere(SVector3 position, float radius, SColor color)
{
    MCameraEntity* cam = getActiveCamera();
    if (!cam) return;

    SMatrix4 vp = cam->getProjectionMatrix(getResolution()) * cam->getViewMatrix();
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(glm::value_ptr(vp));
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glColor4f(color.r, color.g, color.b, color.a);
    const int segments = 32;

    // XY
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segments; ++i) {
        float theta = i * 2.0f * glm::pi<float>() / segments;
        glVertex3f(position.x + radius * cos(theta), position.y + radius * sin(theta), position.z);
    }
    glEnd();

    // XZ
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segments; ++i) {
        float theta = i * 2.0f * glm::pi<float>() / segments;
        glVertex3f(position.x + radius * cos(theta), position.y, position.z + radius * sin(theta));
    }
    glEnd();

    // YZ
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segments; ++i) {
        float theta = i * 2.0f * glm::pi<float>() / segments;
        glVertex3f(position.x, position.y + radius * cos(theta), position.z + radius * sin(theta));
    }
    glEnd();
}

void MGizmos::requestGizmoDraws()
{
    auto sceneManager = MSceneManager::getSceneManagerInstance();
    if (sceneManager == nullptr) return;

    auto scene = sceneManager->getActiveScene();
    if (scene == nullptr) return;

    for (auto root : scene->getRootEntities())
    {
       recursiveGizmoDraws(root);
    }
}

void MGizmos::recursiveGizmoDraws(MSpatialEntity* entity)
{
    if (entity == nullptr)
    {
        return;
    }

    entity->onDrawGizmo();

    for (auto child : entity->getChildren())
    {
        recursiveGizmoDraws(child);
    }
}
