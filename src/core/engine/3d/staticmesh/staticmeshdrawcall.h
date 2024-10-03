//
// Created by ssj5v on 03-10-2024.
//

#ifndef STATICMESHDRAWCALL_H
#define STATICMESHDRAWCALL_H
#include "staticmesh.h"
#include "core/graphics/core/opengldrawcall.h"


class MStaticMeshDrawCall : public MOpenGlDrawCall {
private:
    MStaticMesh* mesh;
    MShader* shader;
public:
    void setStaticMesh(MStaticMesh* mesh);
    void setShader();
    void draw() override;
};



#endif //STATICMESHDRAWCALL_H
