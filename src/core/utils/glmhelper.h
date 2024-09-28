//
// Created by Vishnu Rajendran on 2024-09-28.
//

#ifndef METEOR_ENGINE_GLMHELPER_H
#define METEOR_ENGINE_GLMHELPER_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

typedef glm::vec1 SVector1;
typedef glm::vec2 SVector2;
typedef glm::vec3 SVector3;
typedef glm::vec4 SVector4;

typedef glm::mat2 SMatrix2;
typedef glm::mat3 SMatrix3;
typedef glm::mat4 SMatrix4;

typedef glm::quat SQuaternion;

SVector3 quaternionToEuler(const SQuaternion& q) ;
glm::quat eulerToQuaternion(const glm::vec3& eulerAngles);
#endif //METEOR_ENGINE_GLMHELPER_H
