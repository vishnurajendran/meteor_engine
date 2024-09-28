//
// Created by ssj5v on 26-09-2024.
//

#include "spatial.h"

#include "core/engine/scene/scene.h"
#include "core/engine/scene/scenemanager.h"

const float PI = 3.1515926535898f;
	const float DEG_TO_RAD = PI / 180.0f;

	void makeRootEntity(MSpatialEntity* entity) {
		if (entity->getParent() != nullptr) {
			entity->getParent()->removeChild(entity);
		}

		MSceneManager::getActiveScene()->addToRoot(entity);
	}

	void removeFromRoot(MSpatialEntity* entity, MScene* scene) {
		auto rootEntities = scene->getRootEntities();
		std::vector<MSpatialEntity*>::iterator it = std::find(rootEntities->begin(), rootEntities->end(), entity);
		if (it == rootEntities->end())
			return;

		int diff = it - rootEntities->begin();
		rootEntities->erase(rootEntities->begin() + diff);
	}

	MSpatialEntity::MSpatialEntity(MSpatialEntity* parent) {
		name = "Spatial";
		relativeScale.x = relativeScale.y = 1;
		children = new std::vector<MSpatialEntity*>();
		if (parent == nullptr)
			makeRootEntity(this);
		else
			parent->addChild(this);
		onStart();
	}

	void MSpatialEntity::addChild(MSpatialEntity* entity) {
		std::vector<MSpatialEntity*>::iterator it = std::find(children->begin(), children->end(), entity);
		if (it != children->end())
			return;

		if (entity->getParent() != nullptr)
			entity->getParent()->removeChild(entity);

		this->children->push_back(entity);
		entity->setParent(this);

		// remove from root
		// this is a by-product of un-parenting an entity using removeChild.
		// since the entity becomes a independant, it will be considered as a root entity
		// and appended to the scene root list.
		removeFromRoot(entity, MSceneManager::getActiveScene());
	}

	void MSpatialEntity::removeChild(MSpatialEntity* entity) {
		std::vector<MSpatialEntity*>::iterator it = std::find(children->begin(), children->end(), entity);
		if (it == children->end())
			return;

		int diff = it - children->begin();
		children->erase(children->begin() + diff);
		entity->setParent(nullptr);
		makeRootEntity(entity);
	}

	MSpatialEntity::~MSpatialEntity() {
		if (children == nullptr)
			return;

		for (auto& child : *children) {
			delete child;
		}
	}

	void MSpatialEntity::onStart() {
	}

	void MSpatialEntity::onUpdate(float deltaTime) {

	}

	void MSpatialEntity::onExit() {

	}