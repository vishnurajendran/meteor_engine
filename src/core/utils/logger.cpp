//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include "logger.h"
std::map<int,LogEventHandler> MLogger::listeners;
int MLogger::nextId = 0;

void MLogger::log(SString msg) {
    spdlog::info(msg.str());
    notify(TEXT("LOG: ") + msg);
}

void MLogger::warn(SString warning) {
    spdlog::warn(warning.str());
    notify(TEXT("WRN: ") + warning);
}

void MLogger::error(SString error) {
    spdlog::error(error.str());
    notify(TEXT("ERR: ") + error);
}

int MLogger::subscribe(const LogEventHandler &handler) {
    auto id = nextId;
    listeners[id] = handler;
    nextId++;
    return id;
}

void MLogger::unsubscribe(const int& id) {
    listeners.erase(id);
}

void MLogger::notify(SString msg) {
    for(const auto& [id, listener] : listeners){
        listener(msg);
    }
}
