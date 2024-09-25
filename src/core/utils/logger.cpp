//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include "logger.h"

void MLogger::log(SString msg) {
    spdlog::info(msg.str());
}

void MLogger::warn(SString warning) {
    spdlog::warn(warning.str());
}

void MLogger::error(SString error) {
    spdlog::error(error.str());
}
