//
// Created by ssj5v on 26-09-2024.
//

#include "scene.h"

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

	void MScene::startScene() {
		//logNoFormat("Loading scene");
		for (int i = 0; i < rootEntities.size(); i++) {
			rootEntities.at(i)->onStart();
		}
	}

	void MScene::update(float deltaTime) {
		//logNoFormat("updating scene");
	}

	void MScene::onClose() {
		//logNoFormat("cleaning scene");
		sceneClosing = true;
		for (int i = 0; i < rootEntities.size(); i++) {
			rootEntities.at(i)->onExit();
		}
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

	void MScene::recursivelyLoadEntity(pugi::xml_node currNode, MSpatialEntity* parent) {
		if (currNode == nullptr) {
			return;
		}
		if(MSceneEntityTypeDeserializer::ATTRIB_NODE == SString(currNode.name())) {
			return;
		}

		auto type = currNode.name();
		auto deserializer = MSceneEntityTypeMap::getDeserializer(type);
		if (deserializer == nullptr) {
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

		for (auto child : currNode.children()) {
			recursivelyLoadEntity(child, entity);
		}
	}