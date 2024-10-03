//
// Created by ssj5v on 29-09-2024.
//

#ifndef MESH_H
#define MESH_H

#include <vector>

#include "core/engine/assetmanagement/asset/asset.h"
#include "core/object/object.h"
#include "core/utils/glmhelper.h"

class MStaticMesh;
struct aiScene;
struct aiNode;
struct aiMesh;

class MStaticMeshAsset : public MAsset {
private:
    std::vector<MStaticMesh*> meshes;
public:
    MStaticMeshAsset(const SString& path);
    ~MStaticMeshAsset();
private:
    MStaticMesh* processMesh(aiMesh *mesh);
    void processNode(aiNode *node, const aiScene *scene,std::vector<MStaticMesh*>& meshes);
    void loadMesh(SString path);
};

#endif //MESH_H
