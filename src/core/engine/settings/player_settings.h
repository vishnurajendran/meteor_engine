//
// player_settings.h
//
// Settings the people playing a published game may change, saved as
// PlayerSettings.xml next to the game's executable. Everything else
// (rendering, physics) is baked into the build's .manifest and is not
// editable; see MEngineStatics::applyProjectSettingsFromXml.
//
// Add user-facing options (audio volume, key bindings, ...) here as they
// come.
//

#ifndef PLAYER_SETTINGS_H
#define PLAYER_SETTINGS_H

#include "core/object/object.h"
#include "data/serialized_class_base.h"
#include "core/utils/field_engine_types.h"

class MPlayerSettings : public SerializedClassBase, public MObject {
    DEFINE_OBJECT_SUBCLASS(MPlayerSettings);

    // Window and presentation
    DECLARE_FIELD(resX, int, 1280);
    DECLARE_FIELD(resY, int, 720);
    DECLARE_FIELD(fps, int, 60);
    DECLARE_FIELD(fullscreen, bool, false);
};

#endif // PLAYER_SETTINGS_H
