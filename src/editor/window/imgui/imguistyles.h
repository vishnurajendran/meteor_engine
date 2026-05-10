//
// Created by Vishnu Rajendran on 2024-09-25.
//

#include "core/object/object.h"
#include "imgui.h"

class MEditorThemes : public MObject
{
    DEFINE_OBJECT_SUBCLASS(MEditorThemes)
private:
    MEditorThemes();
    ~MEditorThemes() override = default;
public:
    static void applyDeepDarkTheme();
    static void applyLightTheme();
};


