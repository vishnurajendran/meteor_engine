#pragma once

#ifndef METEOR_ENGINE_SSTRING_H
#define METEOR_ENGINE_SSTRING_H
#include <string>
#include <algorithm>
#include <vector>

#define STR(...) SString(__VA_ARGS__)

struct SString {
private:
    std::string coreStr;

public:
    // Constructors
    SString() : coreStr("") {}
    SString(const char* str) : coreStr(str) {}
    SString(const std::string& str) : coreStr(str) {}

    // Copy constructor
    SString(const SString& other) : coreStr(other.coreStr) {}

    // Move constructor
    SString(SString&& other) noexcept : coreStr(std::move(other.coreStr)) {}

    // Assignment operator
    SString& operator=(const SString& other) {
        if (this != &other) {
            coreStr = other.coreStr;
        }
        return *this;
    }

    // Move assignment operator
    SString& operator=(SString&& other) noexcept {
        coreStr = std::move(other.coreStr);
        return *this;
    }

    // Type conversion operator to std::string
    operator std::string() const {
        return coreStr;
    }

    // Concatenation operator +
    SString operator+(const SString& other) const {
        return SString(coreStr + other.coreStr);
    }

    // Concatenation and assignment operator +=
    SString& operator+=(const SString& other) {
        coreStr += other.coreStr;
        return *this;
    }

    // Comparison operators
    bool operator==(const SString& other) const {
        return coreStr == other.coreStr;
    }

    bool operator!=(const SString& other) const {
        return coreStr != other.coreStr;
    }

    bool operator<(const SString& other) const {
        return coreStr < other.coreStr;
    }

    bool operator>(const SString& other) const {
        return coreStr > other.coreStr;
    }

    // Access operator []
    char& operator[](size_t index) {
        return coreStr[index];
    }

    const char& operator[](size_t index) const {
        return coreStr[index];
    }

    // Append method
    void append(const SString& str) {
        coreStr.append(str.coreStr);
    }

    // Length method
    size_t length() const {
        return coreStr.length();
    }

    // Clear method
    void clear() {
        coreStr.clear();
    }

    // Is empty method
    bool empty() const {
        return coreStr.empty();
    }

    // Substring method
    SString substring(size_t pos, size_t len) const {
        return SString(coreStr.substr(pos, len));
    }

    // Find method
    size_t find(const SString& str) const {
        return coreStr.find(str.coreStr);
    }

    // Split method
    std::vector<SString> split(const SString& delimiter) const {
        std::vector<SString> result;
        size_t start = 0;
        size_t end = 0;
        while ((end = coreStr.find(delimiter.coreStr, start)) != std::string::npos) {
            result.emplace_back(coreStr.substr(start, end - start));
            start = end + delimiter.coreStr.length();
        }
        result.emplace_back(coreStr.substr(start));
        return result;
    }

    // To uppercase
    void toUpperCase() {
        std::transform(coreStr.begin(), coreStr.end(), coreStr.begin(), ::toupper);
    }

    // To lowercase
    void toLowerCase() {
        std::transform(coreStr.begin(), coreStr.end(), coreStr.begin(), ::tolower);
    }

    // Trim method (removes leading and trailing spaces)
    void trim() {
        coreStr.erase(coreStr.begin(), std::find_if(coreStr.begin(), coreStr.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
        coreStr.erase(std::find_if(coreStr.rbegin(), coreStr.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), coreStr.end());
    }

    // Replace method (replaces all occurrences of a substring with another substring)
    void replace(const SString& target, const SString& replacement) {
        size_t pos = 0;
        while ((pos = coreStr.find(target.coreStr, pos)) != std::string::npos) {
            coreStr.replace(pos, target.coreStr.length(), replacement.coreStr);
            pos += replacement.coreStr.length(); // Move past the replacement
        }
    }

    // C_str method for compatibility
    const char* c_str() const {
        return coreStr.c_str();
    }

    std::string str() const {
        return coreStr;
    }

    // Output stream operator <<
    friend std::ostream& operator<<(std::ostream& os, const SString& str) {
        os << str.coreStr;
        return os;
    }

    // Input stream operator >>
    friend std::istream& operator>>(std::istream& is, SString& str) {
        is >> str.coreStr;
        return is;
    }

    std::size_t operator()(const SString& s) const {
        return std::hash<std::string>()(s.str());
    }
};

#endif // METEOR_ENGINE_SSTRING_H
