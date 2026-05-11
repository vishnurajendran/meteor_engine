//
// Created by ssj5v on 11-05-2026.
//

#ifndef EDITOR_SETTINGS_H
#define EDITOR_SETTINGS_H
#include "core/engine/settings/engine_settings.h"


class MEditorSettings : public MEngineSettings {
    DEFINE_OBJECT_CLASS(MEditorSettings)

    DECLARE_FIELD(editorTheme, std::string, "me::midnight")
};



#endif //EDITOR_SETTINGS_H
