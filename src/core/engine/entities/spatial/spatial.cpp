//
// Created by ssj5v on 26-09-2024.
//

#include "spatial.h"

#include "core/engine/scene/scene.h"
#include "core/engine/scene/scenemanager.h"

auto makeRootEntity(MSpatialEntity *entity) -> void {
    if (entity->getParent() != nullptr) {
        entity->getParent()->removeChild(entity);
    }

    MSceneManager::getActiveScene()->addToRoot(entity);
}

void removeFromRoot(MSpatialEntity *entity, MScene *scene) {
    auto rootEntities = scene->getRootEntities();
    std::vector<MSpatialEntity *>::iterator it = std::find(rootEntities.begin(), rootEntities.end(), entity);
    if (it == rootEntities.end())
        return;

    int diff = it - rootEntities.begin();
    rootEntities.erase(rootEntities.begin() + diff);
}

MSpatialEntity::MSpatialEntity(MSpatialEntity *parent) {
    name = "Spatial";
    relativeScale.x = relativeScale.y = 1;
    if (parent == nullptr)
        makeRootEntity(this);
    else
        parent->addChild(this);
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
    removeFromRoot(entity, MSceneManager::getActiveScene());
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

MSpatialEntity::~MSpatialEntity() {
    if (parent != nullptr) {
        for (auto &child: children) {
            parent->addChild(child);
        }
    } else {
        for (auto &child: children) {
            makeRootEntity(child);
        }
    }
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

    // If this entity has a parent, multiply by the parent's model matrix (for hierarchical transformation)
    if (parent) {
        return parent->getModelMatrix() * localModelMatrix;
    }

    return localModelMatrix;
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
}

void MSpatialEntity::onUpdate(float deltaTime) {
}

void MSpatialEntity::onExit() {
}
