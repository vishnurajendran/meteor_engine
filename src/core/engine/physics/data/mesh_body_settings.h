//
// mesh_body_settings.h
//

#ifndef MESH_BODY_SETTINGS_H
#define MESH_BODY_SETTINGS_H

#include <vector>
#include "core/engine/physics/data/default_body_settings.h"
#include "core/utils/glmhelper.h"

// Vertices are in the body's local space (world scale applied by the entity
// before populating this struct). Indices form triangles — every 3 = 1 tri.
//
// Performance note: this struct owns its vertex/index data via std::vector.
// Move it into the factory call rather than copying where possible.
struct SMeshBodySettings
{
    SVector3           position          = {};
    SQuaternion        rotation          = {};
    ECollisionBodyType bodyType          = ECollisionBodyType::StaticBody;
    float              mass              = 0.0f;
    bool               affectedByGravity = false;

    std::vector<SVector3> vertices;
    std::vector<uint32_t> indices;
};

#endif // MESH_BODY_SETTINGS_H