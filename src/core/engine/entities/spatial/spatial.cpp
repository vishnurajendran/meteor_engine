//
// Created by ssj5v on 26-09-2024.
//

#include "spatial.h"

#include "core/engine/scene/scene.h"
#include "core/engine/scene/scenemanager.h"

std::map<SString, MSpatialEntity*> MSpatialEntity::allAliveEntities;
std::vector<MSpatialEntity*> MSpatialEntity::markedForDestruction;

auto makeRootEntity(MSpatialEntity *entity) -> void {
    if (entity->getParent() != nullptr) {
        entity->getParent()->removeChild(entity);
    }

    MSceneManager::getSceneManagerInstance()->getActiveScene()->addToRoot(entity);
}

void removeFromRoot(const MSpatialEntity *entity, MScene *scene) {
    auto rootEntities = scene->getRootEntities();
    const auto it = std::find(rootEntities.begin(), rootEntities.end(), entity);
    if (it == rootEntities.end())
        return;

    rootEntities.erase(it);
}

MSpatialEntity::MSpatialEntity() : MSpatialEntity(nullptr)
{
    // defaults to an empty root level entity.
    //clear before use
    children.clear();
}

MSpatialEntity::MSpatialEntity(MSpatialEntity *parent) {
    name = "Spatial";
    relativeScale.x = relativeScale.y = 1;
    if (parent == nullptr)
        makeRootEntity(this);
    else
        parent->addChild(this);

    addAliveEntity(this);

    //call on start initialise any custom logic.
    onStart();
}

void MSpatialEntity::addChild(MSpatialEntity *entity) {
    auto it = std::ranges::find(children, entity);
    if (it != children.end())
        return;

    if (entity->getParent() != nullptr)
        entity->getParent()->removeChild(entity);

    this->children.push_back(entity);
    entity->setParent(this);

    // remove from root
    // this is a by-product of un-parenting an entity using removeChild.
    // since the entity becomes an independent, it will be considered as a root entity
    // and appended to the scene root list.
    removeFromRoot(entity, MSceneManager::getSceneManagerInstance()->getActiveScene());
}

void MSpatialEntity::removeChild(MSpatialEntity *entity) {
    auto it = std::find(children.begin(), children.end(), entity);
    if (it == children.end())
        return;

    const int diff = it - children.begin();
    children.erase(children.begin() + diff);
    entity->setParent(nullptr);
    makeRootEntity(entity);
}

SMatrix4 MSpatialEntity::getTransformMatrix() const {
    auto modelMatrix = getModelMatrix();
    if (parent) {
        return parent->getModelMatrix() * modelMatrix;
    }
    return modelMatrix;
}

SVector3 MSpatialEntity::getForwardVector() const
{
    return glm::normalize(getRelativeRotation() * SVector3(0.0f, 0.0f, -1.0f));
}

SVector3 MSpatialEntity::getRightVector() const
{
    return getRelativeRotation() * SVector3(1.0f, 0.0f, 0.0f);
}

SVector3 MSpatialEntity::getUpVector() const
{
    return getRelativeRotation() * SVector3(0.0f, 1.0f, 0.0f);
}

MSpatialEntity::~MSpatialEntity() {

    //call onExit to allow game-code to clean up.

}

SVector3 MSpatialEntity::getWorldPosition() const {
    return glm::vec3(modelMatrix[3]);
}

SQuaternion MSpatialEntity::getWorldRotation() const {
    // Extract the 3x3 rotation matrix (upper-left of the 4x4 matrix)
    glm::mat3 rotationMatrix = glm::mat3(modelMatrix);
    rotationMatrix[0] = glm::normalize(rotationMatrix[0]);  // X axis
    rotationMatrix[1] = glm::normalize(rotationMatrix[1]);  // Y axis
    rotationMatrix[2] = glm::normalize(rotationMatrix[2]);  // Z axis
    return glm::quat_cast(rotationMatrix);
}

SMatrix4 MSpatialEntity::getModelMatrix() const {
    // Compute local model matrix from position, rotation, and scale
    SMatrix4 scaleMatrix = glm::scale(SMatrix4(1.0f), relativeScale);
    SMatrix4 rotationMatrix = glm::mat4_cast(relativeRotation);  // Convert quaternion to rotation matrix
    SMatrix4 translationMatrix = glm::translate(SMatrix4(1.0f), relativePosition);
    SMatrix4 localModelMatrix = translationMatrix * rotationMatrix * scaleMatrix;
    return localModelMatrix;
}

void  MSpatialEntity::setWorldPosition(const SVector3& worldPosition) {
    if (parent) {
        // Get the inverse of the parent's transform to convert world position to local space
        SMatrix4 parentTransform = parent->getTransformMatrix();
        SMatrix4 inverseParentTransform = glm::inverse(parentTransform);

        // Transform the world position to the parent's local space
        SVector4 localPosition4 = inverseParentTransform * SVector4(worldPosition, 1.0f);
        relativePosition = SVector3(localPosition4); // Convert back to 3D vector
    } else {
        // If no parent, directly set world position as relative
        relativePosition = worldPosition;
    }
    updateTransforms();
}

void MSpatialEntity::setWorldRotation(const SQuaternion& worldRotation) {
    if (parent) {
        // Calculate the relative rotation using the parent's world rotation
        SQuaternion parentWorldRotation = parent->getWorldRotation();
        relativeRotation = glm::inverse(parentWorldRotation) * worldRotation;
    } else {
        // If no parent, directly set world rotation as relative
        relativeRotation = worldRotation;
    }
    updateTransforms();
}

void MSpatialEntity::updateTransforms() {
    // Update this entity's model matrix
    modelMatrix = getModelMatrix();
    // Recursively update children's transformations
    for (auto child : children) {
        if (child != nullptr) {
            child->updateTransforms();
        }
    }
    //MLOG("Updating children transforms");
}

void MSpatialEntity::onStart() {
    //will be overriden for code
}

void MSpatialEntity::onUpdate(float deltaTime) {
    //will be overriden for code
}

void MSpatialEntity::onExit()
{
    // will be overriden for code
}
void MSpatialEntity::onDrawGizmo()
{
    // will be overriden for gizmos
}

void MSpatialEntity::updateAllSceneEntities(float deltaTime)
{
    for (const auto& pair : allAliveEntities)
    {
        pair.second->onUpdate(deltaTime);
    }
}

void MSpatialEntity::destroy(MSpatialEntity* entity)
{
    if (entity == nullptr) return;

    entity->onExit();
    removeAliveEntity(entity);
    if (entity->parent != nullptr) {
        for (auto &child: entity->children) {
            entity->parent->addChild(child);
        }
        entity->parent->removeChild(entity);
        //remove from scene roots.
    } else {
        for (auto &child: entity->children) {
            makeRootEntity(child);
        }
        auto roots = MSceneManager::getSceneManagerInstance()->getActiveScene()->getRootEntities();
        auto it = std::find(roots.begin(), roots.end(), entity);
        if (it != roots.end())
        {
            roots.erase(it);
        }
    }

    markedForDestruction.push_back(entity);
}

void MSpatialEntity::destroyMarked()
{
    for (auto marked : markedForDestruction)
    {
        delete marked;
    }
    markedForDestruction.clear();
}

void MSpatialEntity::addAliveEntity(MSpatialEntity* entity)
{
    allAliveEntities[entity->getGUID()] = entity;
}

void MSpatialEntity::removeAliveEntity(MSpatialEntity* entity)
{
    if (allAliveEntities.contains(entity->getGUID()))
    {
        allAliveEntities.erase(entity->getGUID());
    }
}
