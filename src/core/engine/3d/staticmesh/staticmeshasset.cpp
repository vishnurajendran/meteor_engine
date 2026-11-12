//
// Created by ssj5v on 29-09-2024.
//

#include "staticmeshasset.h"

#include <assimp/scene.h>
#include <assimp/Importer.hpp>

#include <algorithm>
#include <limits>

#include "staticmesh.h"
#include "pugixml.hpp"
#include "assimp/postprocess.h"
#include "core/engine/assetmanagement/source/asset_sources.h"
#include "core/engine/assetmanagement/source/assimp_asset_io.h"
#include "core/utils/logger.h"
#include "core/memory/mnew.h"

MStaticMeshAsset::MStaticMeshAsset(const SString& path) : MAsset(path) {
    loadMesh();
}

MStaticMeshAsset::MStaticMeshAsset(const SString& path, const pugi::xml_node& importSettings) : MAsset(path) {
    loadImportSettings(importSettings);   // before loading: the scale is baked into the vertices
    loadMesh();
}

MStaticMeshAsset::~MStaticMeshAsset() {
    for(auto mesh : meshes) {
        delete mesh;
    }

}

MStaticMesh* MStaticMeshAsset::processMesh(aiMesh *mesh, const SVector3& origin, float scale) {
    std::vector<SVertex> vertices;
    std::vector<unsigned int> indices;

    bool hasTextureCoords = mesh->mTextureCoords[0] != nullptr;
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        SVertex vertex;
        const glm::vec3 source(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        // Shift to the chosen pivot, then scale. Uniform: normals are unaffected.
        vertex.Position = (source - origin) * scale;
        if (mesh->mNormals)   // GenNormals skips point/line meshes
            vertex.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        if(hasTextureCoords)
            vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        vertices.push_back(vertex);
    }

    // Process faces to retrieve indices. Only triangles are drawn - OBJ/FBX can
    // carry point or line elements, which would scramble a GL_TRIANGLES index list.
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        const aiFace& face = mesh->mFaces[i];
        if (face.mNumIndices != 3)
            continue;
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    return mnew<MStaticMesh>(vertices, indices);
}

void MStaticMeshAsset::processNode(aiNode *node, const aiScene *scene, const SVector3& origin,
                                   std::vector<MStaticMesh*>& meshes) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, origin, scaleFactor));
    }

    // After processing meshes, recursively process each child node
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene, origin, meshes);
    }
}

// Bounds of every vertex in the scene. After PreTransformVertices all meshes
// are already in the file's root space, so this is the whole model.
static bool computeSceneBounds(const aiScene* scene, SVector3& outMin, SVector3& outMax) {
    outMin = SVector3(std::numeric_limits<float>::max());
    outMax = SVector3(std::numeric_limits<float>::lowest());
    bool any = false;
    for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
        const aiMesh* mesh = scene->mMeshes[m];
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            const SVector3 v(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
            outMin = glm::min(outMin, v);
            outMax = glm::max(outMax, v);
            any = true;
        }
    }
    if (!any) outMin = outMax = SVector3(0.0f);
    return any;
}

void MStaticMeshAsset::loadMesh() {

    Assimp::Importer importer;
    importer.SetIOHandler(new MAssimpAssetIOSystem(MAssetSources::getActive()));   // importer owns it

    // PreTransformVertices - bakes each node's full parent→child transform into
    // its mesh's vertices, then merges meshes that share a material. Without it,
    // sub-meshes stay in their local space and pile up at the origin.
    // Static meshes only - it discards the node hierarchy, bones and animations,
    // so a future skeletal importer must NOT use this flag.
    // JoinIdenticalVertices - OBJ (and some FBX) arrive un-indexed, one vertex
    // per face corner; welding them cuts the vertex count roughly 3-6x.
    // GlobalScale - applies the file's own unit scale, which importers only
    // record: FBX declares centimetres via UnitScaleFactor, so this brings it
    // to metres like glTF/OBJ (scale 1 for those). Without the step it is ignored.
    constexpr auto flags = aiProcess_Triangulate
                         | aiProcess_JoinIdenticalVertices
                         | aiProcess_GlobalScale
                         | aiProcess_GenNormals
                         | aiProcess_FlipUVs
                         | aiProcess_PreTransformVertices;
    const aiScene* scene = importer.ReadFile(path.c_str(), flags);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        MERROR(STR("Error (Assimp) ") + importer.GetErrorString() + " - " + path);
        valid = !meshes.empty();
        return;
    }

    // The pivot is picked from the whole model's bounds, so it is worked out
    // before any vertices are built.
    SVector3 boundsMin, boundsMax;
    computeSceneBounds(scene, boundsMin, boundsMax);

    SVector3 origin(0.0f);
    switch (pivot) {
        case EMeshPivot::Center:
            origin = (boundsMin + boundsMax) * 0.5f;
            break;
        case EMeshPivot::BottomCenter:
            origin = SVector3((boundsMin.x + boundsMax.x) * 0.5f, boundsMin.y, (boundsMin.z + boundsMax.z) * 0.5f);
            break;
        case EMeshPivot::FileOrigin:
        default:
            break;
    }

    // Build into a temporary list so a failed reload keeps the old meshes.
    std::vector<MStaticMesh*> loaded;
    processNode(scene->mRootNode, scene, origin, loaded);
    if (loaded.empty()) {
        MERROR(STR("MStaticMeshAsset:: no meshes found in ") + path);
        valid = !meshes.empty();
        return;
    }

    sourceBoundsMin = boundsMin - origin;
    sourceBoundsMax = boundsMax - origin;

    for (auto mesh : meshes)
        delete mesh;
    meshes = std::move(loaded);
    valid = true;
}

// -- Import settings persistence -----------------------------------------------
//   <asset_id id="...">
//       <meshImport>
//           <scaleFactor>0.01</scaleFactor>
//           <pivot>2</pivot>            0 file origin, 1 center, 2 bottom center
//       </meshImport>
//   </asset_id>

void MStaticMeshAsset::loadImportSettings(const pugi::xml_node& node) {
    if (!node) return;
    if (auto n = node.child("scaleFactor"))
        setScaleFactor(n.text().as_float(1.0f));
    if (auto n = node.child("pivot"))
        pivot = static_cast<EMeshPivot>(std::clamp(n.text().as_int(0), 0, 2));
}

bool MStaticMeshAsset::saveImportSettings() {
    auto target = MAssetSources::getWritable();
    if (!target) {
        MERROR("MStaticMeshAsset::saveImportSettings - active asset source is read-only: " + path);
        return false;
    }

    // Read the existing meta so the GUID and anything else in it is kept.
    pugi::xml_document doc;
    target->readMeta(path, doc);

    auto root = doc.child("asset_id");
    if (!root) {
        MERROR("MStaticMeshAsset::saveImportSettings - meta file missing <asset_id> root: " + path);
        return false;
    }

    root.remove_child("meshImport");
    auto settings = root.append_child("meshImport");
    settings.append_child("scaleFactor").text().set(scaleFactor);
    settings.append_child("pivot").text().set(static_cast<int>(pivot));

    return target->writeMeta(path, doc);
}

std::vector<MStaticMesh *> MStaticMeshAsset::getMeshes() const {
    return meshes;
}
