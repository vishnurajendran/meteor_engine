//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include "logger.h"
std::map<int,LogEventHandler> MLogger::listeners;
int MLogger::nextId = 0;
SString MLogger::lastMsg = "";

void MLogger::log(SString msg) {
    lastMsg = msg;
    spdlog::info(msg.str());
    notify(STR("LOG: ") + msg);
}

void MLogger::warn(SString warning) {
    lastMsg = warning;
    spdlog::warn(warning.str());
    notify(STR("WRN: ") + warning);
}

void MLogger::error(SString error) {
    lastMsg = error;
    spdlog::error(error.str());
    notify(STR("ERR: ") + error);
}

int MLogger::subscribe(const LogEventHandler &handler) {
    auto id = nextId;
    listeners[id] = handler;
    nextId++;
    return id;
}

void MLogger::unsubscribe(const int& id)
{
    listeners.erase(id);
}

SString MLogger::getLastMessage()
{
    return lastMsg;
}

void MLogger::notify(SString msg) {
    for(const auto& [id, listener] : listeners){
        listener(msg);
    }
}
