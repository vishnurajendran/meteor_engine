//
// Created by ssj5v on 24-05-2026.
//

#ifndef EDITOR_PATHS_H
#define EDITOR_PATHS_H
#include "core/utils/sstring.h"

struct SEditorPaths {
    static constexpr const char* DIR_ROOT = ".engine_data";
    static constexpr const char* DIR_THUMBNAILS_CACHE = ".engine_data/thumbnail_cache";
    static constexpr const char* DIR_TEMP = ".engine_data/temp";
    static constexpr const char* DIR_TEMPLATES_PATH = "templates";

    static constexpr const char* EXTENSION_SHADER   = ".mesl";
    static constexpr const char* EXTENSION_MATERIAL = ".material";
    static constexpr const char* EXTENSION_SKYBOX   = ".skybox";

    static constexpr const char* TEMPLATE_SKYBOX_FILE   = "skybox.skybox";

    static constexpr const char* TEMPLATE_SHADER_LIT_FILE   = "shader_lit.mesl";
    static constexpr const char* TEMPLATE_SHADER_UNLIT_FILE   = "shader_unlit.mesl";
    static constexpr const char* TEMPLATE_SHADER_UNLIT_COLOR_FILE   = "shader_unlit_color.mesl";
    static constexpr const char* TEMPLATE_SHADER_TOONLIT_FILE   = "shader_toon.mesl";
};

#endif //EDITOR_PATHS_H
