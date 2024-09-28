//
// Created by Vishnu Rajendran on 2024-09-28.
//

#include "glmhelper.h"

SVector3 quaternionToEuler(const SQuaternion& q) {
    //Convert quaternion to Euler angles in radians
    glm::vec3 eulerAngles = glm::eulerAngles(q);
    // Convert radians to degrees
    glm::vec3 eulerAnglesDegrees = glm::degrees(eulerAngles);
    return eulerAnglesDegrees;
}

glm::quat eulerToQuaternion(const glm::vec3& eulerAngles) {
    // Convert degrees to radians
    glm::vec3 eulerAnglesRadians = glm::radians(eulerAngles);
    // Convert Euler angles (in radians) to quaternion
    glm::quat quaternion = glm::quat(eulerAnglesRadians);
    return quaternion;
}