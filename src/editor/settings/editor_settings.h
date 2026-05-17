//
// Created by ssj5v on 11-05-2026.
//

#ifndef EDITOR_SETTINGS_H
#define EDITOR_SETTINGS_H

#include "core/utils/field_engine_types.h"
#include "core/utils/glmhelper.h"
#include "core/engine/settings/engine_settings.h"


class MEditorSettings : public MEngineSettings {
    DEFINE_OBJECT_CLASS(MEditorSettings)

    DECLARE_FIELD(editorTheme, std::string, "me::midnight")
    DECLARE_FIELD(lastOpenedScene, std::string, "");
    DECLARE_FIELD(lastEdCameraPos, SVector3, SVector3(0,0,0));
    DECLARE_FIELD(lastEdCameraRot, SQuaternion, glm::identity<SQuaternion>());

};



#endif //EDITOR_SETTINGS_H
