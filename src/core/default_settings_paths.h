//
// Created by ssj5v on 29-05-2026.
//

#ifndef DEFAULT_SETTINGS_PATHS_H
#define DEFAULT_SETTINGS_PATHS_H
#include "utils/sstring.h"

static const SString DEFAULT_SETTINGS_PATH = ".engine_data/settings/";
static const SString DEFAULT_PHYSICS_SETTINGS_PATH = SString::format("{0}{1}", DEFAULT_SETTINGS_PATH, "PhysicsSettings.xml");


#endif //DEFAULT_SETTINGS_PATHS_H
