//
// Created by Vishnu Rajendran on 2024-09-18.
//

#include "guid.h"
#include <random>
#include <sstream>
#include <iomanip>

SString SGuid::newGUID() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dis(0, 0xFFFFFFFF);

    std::stringstream ss;
    ss << std::hex << std::setfill('0');

    // Generate and format UUID (version 4)
    ss << std::setw(8) << dis(gen) << "-";
    ss << std::setw(4) << (dis(gen) & 0xFFFF) << "-";
    ss << std::setw(4) << ((dis(gen) & 0x0FFF) | 0x4000) << "-";  // 4 in the high nibble
    ss << std::setw(4) << ((dis(gen) & 0x3FFF) | 0x8000) << "-";  // 8, 9, A, or B in the high nibble
    ss << std::setw(12) << dis(gen) << dis(gen);
    return ss.str();
}
