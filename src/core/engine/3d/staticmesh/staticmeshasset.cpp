//
// Created by ssj5v on 29-09-2024.
//

#include "staticmeshasset.h"

#include <assimp/scene.h>
#include <assimp/Importer.hpp>

#include "staticmesh.h"
#include "assimp/postprocess.h"
#include "core/engine/assetmanagement/source/asset_sources.h"
#include "core/engine/assetmanagement/source/assimp_asset_io.h"
#include "core/utils/logger.h"

MStaticMeshAsset::MStaticMeshAsset(const SString& path) : MAsset(path) {
    loadMesh();
}

MStaticMeshAsset::~MStaticMeshAsset() {
    for(auto mesh : meshes) {
        delete mesh;
    }

}

MStaticMesh* MStaticMeshAsset::processMesh(aiMesh *mesh) {
    std::vector<SVertex> vertices;
    std::vector<unsigned int> indices;

    bool hasTextureCoords = mesh->mTextureCoords[0] != nullptr;
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        SVertex vertex;
        vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        if (mesh->mNormals)   // GenNormals skips point/line meshes
            vertex.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        if(hasTextureCoords)
            vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
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

void MStaticMeshAsset::loadMesh() {

    Assimp::Importer importer;
    importer.SetIOHandler(new MAssimpAssetIOSystem(MAssetSources::getActive()));   // importer owns it

    // PreTransformVertices - bakes each node's full parent→child transform into
    // its mesh's vertices, then merges meshes that share a material. Without it,
    // sub-meshes stay in their local space and pile up at the origin.
    // Static meshes only - it discards the node hierarchy, bones and animations,
    // so a future skeletal importer must NOT use this flag.
    constexpr auto flags = aiProcess_Triangulate
                         | aiProcess_GenNormals
                         | aiProcess_FlipUVs
                         | aiProcess_PreTransformVertices;
    const aiScene* scene = importer.ReadFile(path.c_str(), flags);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        MERROR(STR("Error (Assimp) ") + importer.GetErrorString() + " - " + path);
        valid = !meshes.empty();
        return;
    }

    // Build into a temporary list so a failed reload keeps the old meshes.
    std::vector<MStaticMesh*> loaded;
    processNode(scene->mRootNode, scene, loaded);
    if (loaded.empty()) {
        MERROR(STR("MStaticMeshAsset:: no meshes found in ") + path);
        valid = !meshes.empty();
        return;
    }

    for (auto mesh : meshes)
        delete mesh;
    meshes = std::move(loaded);
    valid = true;
}

std::vector<MStaticMesh *> MStaticMeshAsset::getMeshes() const {
    return meshes;
}
