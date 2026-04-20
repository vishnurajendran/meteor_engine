//
// Created by ssj5v on 26-09-2024.
//

#include "scene.h"

#include "core/engine/entities/spatial/spatial.h"
#include "serialisation/sceneentitytypemap.h"

const SString MScene::VALID_SCENE_FILE_XML_TAG = "mscene";
const SString MScene::SCENE_NAME_TAG = "name";


	MScene::MScene() {
		name = "Untitled";
		startScene();
	}

	MScene::~MScene() {
		rootEntities.clear();
	}

	void MScene::startScene() const
	{
		//nothing here for now
	}

	void MScene::update(float deltaTime)
	{
        destroyMarked();

		for (const auto [obj, ptr]: allAliveEntities)
		{
		    if (ptr == nullptr) continue;

		    // call start if the entity was not started.
		    if (!ptr->hasStarted())
		        ptr->onStart();

            if (ptr->getCanTick())
                ptr->onUpdate(deltaTime);
		}
	}

	void MScene::close()
	{
	    MLOG(SString::format("Initiating scene closure...", name));
		sceneClosing = true;
		// Destroy all alive entities — this calls onExit() on each, which
		// unregisters lights from MLightSystemManager, removes skybox from
	    // MSkyboxQueue, etc. Without this the static allAliveEntities map
	    // keeps them alive and they keep rendering into the next scene.

	    for (const auto& [obj, ptr] : getAllEntities())
			if (ptr.get())
			    ptr->destroy();

	    destroyMarked();
		rootEntities.clear();
	    MLOG(STR("Scene closed"));
	}

    void MScene::markForDestroy(MSpatialEntity* entity)
	{
	    if (entity == nullptr) return;
        if (markedForDestroy.contains(entity)) return;
	    markedForDestroy.insert(entity);
	}

    void MScene::destroyMarked()
	{
	    if (markedForDestroy.empty()) return;
	    MLOG(SString::format("Marked for destroy {0}", std::to_string(markedForDestroy.size())));
	    for (auto entity : markedForDestroy)
	    {
	        // Re-parent or promote children before the entity disappears.
	        if (auto* myParent = entity->getParent())
	        {
	            for (auto* child : entity->getChildren())
	            {
	                entity->setParent(myParent);
	            }
	            myParent->removeChild(entity);
	        }
	        auto  it    = std::ranges::find(rootEntities, entity);
	        if (it != rootEntities.end()) rootEntities.erase(it);
	        allAliveEntities.erase(entity);
	    }

	    markedForDestroy.clear();
	}

	void MScene::addToRoot(MSpatialEntity* entity) {
		if (entity->getParent() != nullptr)
			return;
		auto it = std::find(rootEntities.begin(), rootEntities.end(), entity);
		if (it != rootEntities.end())
			return;
		rootEntities.push_back(entity);
	}

	bool MScene::tryParse(pugi::xml_document* doc) {
		if (doc == nullptr)
			return false;

		if (!doc->child(VALID_SCENE_FILE_XML_TAG.c_str()))
			return false;

		auto scene_xml = doc->child(VALID_SCENE_FILE_XML_TAG.c_str());
		if(scene_xml.attribute(SCENE_NAME_TAG.c_str()) != nullptr) {
			name = scene_xml.attribute(SCENE_NAME_TAG.c_str()).value();
		}
		else
			name = "Untitled";

		for (auto& child : scene_xml.children()) {
			recursivelyLoadEntity(child, nullptr);
		}

		for(auto& entity : rootEntities) {
			entity->updateTransforms();
		}

		startScene();
		return true;
	}

	void MScene::recursivelyLoadEntity(pugi::xml_node currNode, MSpatialEntity* parent)
    {
        if (currNode == nullptr)
        {
            return;
        }
        if (MSceneEntityTypeDeserializer::ATTRIB_NODE == SString(currNode.name()))
        {
            return;
        }

        auto type = currNode.name();
        auto deserializer = MSceneEntityTypeMap::getDeserializer(type);
        if (deserializer == nullptr)
        {
            MWARN("cannot find deserializer for {}, skipping this branch." + std::string(type));
            return;
        }

        MSpatialEntity* entity = deserializer->deserialize(currNode);
        if (entity == nullptr)
        {
            MWARN("deserialized entity for {} is nullptr, skipping this branch." + std::string(type));
            return;
        }
        if (parent != nullptr)
            parent->addChild(entity);

        for (auto child : currNode.children())
        {
            recursivelyLoadEntity(child, entity);
        }
    }

    bool MScene::registerEntity(MSpatialEntity* entity)
	{
	    if (entity == nullptr) return false;

	    addToRoot(entity);
	    allAliveEntities[entity] = entity;
	    entity->onCreate();
	    return true;
	}
