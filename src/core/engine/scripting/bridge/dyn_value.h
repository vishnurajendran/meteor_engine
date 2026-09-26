//
// Created by ssj5v on 05-06-2026.
//

#ifndef DYN_VALUE_H
#define DYN_VALUE_H
#include <string>
#include <variant>

enum class EDynValueType
{
    NoValue,
    Integer,
    Float,
    Bool,
    String,
};

struct SDynValue
{
    EDynValueType type;
    std::variant<bool, int, float, std::string> value;

public:
    bool IsType(EDynValueType t) const { return type == t; }
    void setBool(bool v) { value = v; type = EDynValueType::Bool; }
    void setInt(int v) { value = v; type = EDynValueType::Integer; }
    void setFloat(float v) { value = v; type = EDynValueType::Float; }
    void setString(const std::string& v) { value = v; type = EDynValueType::String; }
};
#endif //DYN_VALUE_H
