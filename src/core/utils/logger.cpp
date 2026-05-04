//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include "logger.h"

std::map<int,LogEventHandler>& MLogger::getListeners() {
    static std::map<int,LogEventHandler> listeners;
    return listeners;
}

int& MLogger::getNextId() {
    static int nextId = 0;
    return nextId;
}

SString& MLogger::getLastMsg() {
    static SString lastMsg = "";
    return lastMsg;
}

void MLogger::log(SString msg)
{
    getLastMsg() = msg;
    spdlog::info(msg.str());
    notify(STR("LOG: ") + msg);
}
void MLogger::verbose(SString msg)
{
#ifdef VERBOSE_LOGGING
    log(msg);
#endif
}

void MLogger::warn(SString warning) {
    getLastMsg() = warning;
    spdlog::warn(warning.str());
    notify(STR("WRN: ") + warning);
}

void MLogger::error(SString error) {
    getLastMsg() = error;
    spdlog::error(error.str());
    notify(STR("ERR: ") + error);
}

int MLogger::subscribe(const LogEventHandler &handler) {
    auto id = getNextId()++;
    getListeners()[id] = handler;
    return id;
}

void MLogger::unsubscribe(const int& id)
{
    getListeners().erase(id);
}

SString MLogger::getLastMessage()
{
    return getLastMsg();
}

void MLogger::notify(SString msg) {
    for(const auto& [id, listener] : getListeners()){
        if (listener)
            listener(msg);
    }
}