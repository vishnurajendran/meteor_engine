//
// Created by ssj5v on 29-09-2024.
//

#include "staticmeshasset.h"

#include <assimp/scene.h>
#include <assimp/Importer.hpp>

#include "staticmesh.h"
#include "assimp/postprocess.h"
#include "core/utils/logger.h"

MStaticMeshAsset::MStaticMeshAsset(const SString& path) : MAsset(path) {
    this->path= path;
    loadMesh(path);
}

MStaticMeshAsset::~MStaticMeshAsset() {
    for(auto mesh : meshes) {
        delete mesh;
    }
}

MStaticMesh* MStaticMeshAsset::processMesh(aiMesh *mesh) {
    std::vector<SVertex> vertices;
    std::vector<unsigned int> indices;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        SVertex vertex;
        vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        vertex.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        vertices.push_back(vertex);
    }
    // Process faces to retrieve indices
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    return new MStaticMesh(vertices, indices);
}

void MStaticMeshAsset::processNode(aiNode *node, const aiScene *scene, std::vector<MStaticMesh*>& meshes) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh));
    }

    // After processing meshes, recursively process each child node
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene, meshes);
    }
}

void MStaticMeshAsset::loadMesh(SString path) {
    Assimp::Importer importer;
    constexpr auto flags = aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals;
    const aiScene* scene = importer.ReadFile(path, flags);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        MERROR(STR("Error (Assimp) ") + importer.GetErrorString());
        return;
    }
    // Start the recursive processing from the root node
    processNode(scene->mRootNode, scene, meshes);
    valid = !meshes.empty();
}
