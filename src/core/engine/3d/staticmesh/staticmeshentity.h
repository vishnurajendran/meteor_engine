//
// Created by ssj5v on 05-10-2024.
//

#ifndef STATICMESHENTITY_H
#define STATICMESHENTITY_H
#include "core/engine/entities/spatial/spatial.h"
#include "core/graphics/core/drawcallsubmitable.h"


class MStaticMeshDrawCall;
class MMaterial;
class MStaticMeshAsset;

class MStaticMeshEntity : public MSpatialEntity, public IDrawCallSubmitable {
public:
    void raiseDrawCall() override;

private:
    MStaticMeshAsset* staticMeshAsset = nullptr;
    MMaterial* materialInstance = nullptr;
    MStaticMeshDrawCall* drawCall = nullptr;
public:
    MStaticMeshEntity();
    ~MStaticMeshEntity() override;
    void setStaticMeshAsset(MStaticMeshAsset* asset);
    void setMaterial(MMaterial *material);
};

#endif //STATICMESHENTITY_H
