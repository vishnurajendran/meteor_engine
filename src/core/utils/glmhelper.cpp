//
// Created by Vishnu Rajendran on 2024-09-28.
//

#include "glmhelper.h"

SVector3 quaternionToEuler(const SQuaternion& q) {
    glm::vec3 eulerAngles = glm::eulerAngles(q);
    return glm::degrees(eulerAngles);
}

glm::quat eulerToQuaternion(const glm::vec3& eulerAngles) {
    glm::vec3 eulerAnglesRadians = SVector3(glm::radians(eulerAngles.x), glm::radians(eulerAngles.y), glm::radians(eulerAngles.z));
    glm::quat quaternion = glm::quat(eulerAnglesRadians);
    return quaternion;
}