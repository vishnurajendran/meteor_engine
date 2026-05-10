//
// Created by ssj5v on 05-10-2024.
//

#ifndef VIEWMANAGEMENT_H
#define VIEWMANAGEMENT_H
#include <vector>

#include "camera_spatial_entity.h"
#include "core/object/object.h"


class MViewManagement : public MObject {
    DEFINE_OBJECT_SUBCLASS(MViewManagement)
private:
    static std::vector<MCameraEntity*> cameras;
public:
    static void addCamera(MCameraEntity* camera);
    static void removeCamera(MCameraEntity* camera);
    static std::vector<MCameraEntity*>& getCameras();
    static void updateCameraOrder();
};



#endif //VIEWMANAGEMENT_H
