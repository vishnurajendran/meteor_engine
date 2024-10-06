//
// Created by ssj5v on 27-09-2024.
//
#include "serialisation_utils.h"

#include "logger.h"
#include "str_extensions.h"

SString serializeVector (SVector3 vector) {
    return STR("(" + std::to_string(vector.x) + "," + std::to_string(vector.y) + "," + std::to_string(vector.z) + ")");
}

bool parseVector2 (SString str, SVector2& out) {
    if (str[0] != '(') {
        MERROR("vector parse failed, reason: expected ( at the begining.)");
        return false;
    }
    if (str[str.length() - 1] != ')') {
        MERROR("vector parse failed, reason: expected ) at the end.");
        return false;
    }

    str = string_utils::replace<std::string>(str,"(","");
    str = string_utils::replace<std::string>(str, ")", "");

    if (string_utils::numberOfOccurence<std::string>(str,",") != 1) {
        MERROR("vector parse failed, reason: too many or too few components");
        return false;
    }
    auto split = string_utils::split<std::string, std::string>(str, ",");
    if (!(string_utils::isInt(split[0]) || string_utils::isFloat(split[0]))) {
        MERROR("vector parse failed, reason: " + std::string(split[0]) + ": x component is not a number");
        return false;
    }
    if (!(string_utils::isInt(split[1]) || string_utils::isFloat(split[1]))) {
        MERROR("vector parse failed, reason:" + std::string(split[1]) + ": y component is not a number");
        return false;
    }

    out.x = std::stof(split[0]);
    out.y = std::stof(split[1]);
    return true;
}

bool parseVector3 (SString str, SVector3& out) {
    if (str[0] != '(') {
        MERROR("vector parse failed, reason: expected ( at the begining.)");
        return false;
    }
    if (str[str.length() - 1] != ')') {
        MERROR("vector parse failed, reason: expected ) at the end.");
        return false;
    }

    str = string_utils::replace<std::string>(str,"(","");
    str = string_utils::replace<std::string>(str, ")", "");

    if (string_utils::numberOfOccurence<std::string>(str,",") != 2) {
        MERROR("vector parse failed, reason: too many or too few components");
        return false;
    }
    auto split = string_utils::split<std::string, std::string>(str, ",");
    if (!(string_utils::isInt(split[0]) || string_utils::isFloat(split[0]))) {
        MERROR("vector parse failed, reason: " + std::string(split[0]) + ": x component is not a number");
        return false;
    }
    if (!(string_utils::isInt(split[1]) || string_utils::isFloat(split[1]))) {
        MERROR("vector parse failed, reason:" + std::string(split[1]) + ": y component is not a number");
        return false;
    }
    if (!(string_utils::isInt(split[2]) || string_utils::isFloat(split[2]))) {
        MERROR("vector parse failed, reason:" + std::string(split[2]) + ": z component is not a number");
        return false;
    }

    out.x = std::stof(split[0]);
    out.y = std::stof(split[1]);
    out.z = std::stof(split[2]);
    return true;
}

bool parseVector4 (SString str, SVector4& out) {
    if (str[0] != '(') {
        MERROR("vector parse failed, reason: expected ( at the begining.)");
        return false;
    }
    if (str[str.length() - 1] != ')') {
        MERROR("vector parse failed, reason: expected ) at the end.");
        return false;
    }

    str = string_utils::replace<std::string>(str,"(","");
    str = string_utils::replace<std::string>(str, ")", "");

    if (string_utils::numberOfOccurence<std::string>(str,",") != 3) {
        MERROR("vector parse failed, reason: too many or too few components");
        return false;
    }
    auto split = string_utils::split<std::string, std::string>(str, ",");
    if (!(string_utils::isInt(split[0]) || string_utils::isFloat(split[0]))) {
        MERROR("vector parse failed, reason: " + std::string(split[0]) + ": x component is not a number");
        return false;
    }
    if (!(string_utils::isInt(split[1]) || string_utils::isFloat(split[1]))) {
        MERROR("vector parse failed, reason:" + std::string(split[1]) + ": y component is not a number");
        return false;
    }
    if (!(string_utils::isInt(split[2]) || string_utils::isFloat(split[2]))) {
        MERROR("vector parse failed, reason:" + std::string(split[2]) + ": z component is not a number");
        return false;
    }
    if (!(string_utils::isInt(split[3]) || string_utils::isFloat(split[3]))) {
        MERROR("vector parse failed, reason:" + std::string(split[3]) + ": w component is not a number");
        return false;
    }

    out.x = std::stof(split[0]);
    out.y = std::stof(split[1]);
    out.z = std::stof(split[2]);
    out.w = std::stof(split[3]);
    return true;
}

SString serializeQuaternion (SQuaternion quaternion) {
    return STR("(" + std::to_string(quaternion.x) + "," + std::to_string(quaternion.y) + "," + std::to_string(quaternion.z) + "," + std::to_string(quaternion.w) + ")");
}

bool parseQuaternion (SString str, SQuaternion& out) {
    if (str[0] != '(') {
        MERROR("quaternion parse failed, reason: expected ( at the begining.)");
        return false;
    }
    if (str[str.length() - 1] != ')') {
        MERROR("quaternion parse failed, reason: expected ) at the end.");
        return false;
    }

    str = string_utils::replace<std::string>(str,"(","");
    str = string_utils::replace<std::string>(str, ")", "");

    if (string_utils::numberOfOccurence<std::string>(str,",") != 3) {
        MERROR("quaternion parse failed, reason: too many or too few components");
        return false;
    }
    auto split = string_utils::split<std::string, std::string>(str, ",");
    if (!(string_utils::isInt(split[0]) || string_utils::isFloat(split[0]))) {
        MERROR("quaternion parse failed, reason: " + std::string(split[0]) + ": x component is not a number");
        return false;
    }
    if (!(string_utils::isInt(split[1]) || string_utils::isFloat(split[1]))) {
        MERROR("quaternion parse failed, reason:" + std::string(split[1]) + ": y component is not a number");
        return false;
    }
    if (!(string_utils::isInt(split[2]) || string_utils::isFloat(split[2]))) {
        MERROR("quaternion parse failed, reason:" + std::string(split[2]) + ": y component is not a number");
        return false;
    }
    if (!(string_utils::isInt(split[3]) || string_utils::isFloat(split[3]))) {
        MERROR("quaternion parse failed, reason:" + std::string(split[3]) + ": y component is not a number");
        return false;
    }

    out.x = std::stof(split[0]);
    out.y = std::stof(split[1]);
    out.z = std::stof(split[2]);
    out.w = std::stof(split[3]);

    return true;
}