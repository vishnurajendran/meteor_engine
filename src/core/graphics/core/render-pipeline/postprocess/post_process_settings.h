//
// Post-process effect settings - the data half of an effect.
//

#pragma once
#ifndef POST_PROCESS_SETTINGS_H
#define POST_PROCESS_SETTINGS_H

#include <string>
#include <vector>

#include "data/serialized_class_base.h"

// Settings are plain data: DECLARE_FIELD members, saved with the scene by
// MPostProcessStackEntity. Each effect owns one instance; the scene only
// ever gets a pointer to it through MPostProcessStack::getSettings().
//
// Supported field types: bool, int, float, SVector3 (what the inspector and
// the by-name accessors handle). SVector3 fields need
// "core/utils/field_engine_types.h" included for save/load.
//
//   class SVignetteSettings : public SPostProcessEffectSettings {
//       DECLARE_FIELD(intensity, float, 0.3f)
//   };
class SPostProcessEffectSettings : public SerializedClassBase
{
    DECLARE_FIELD(enabled, bool, true)

public:
    // ---- Inspector hints ----------------------------------------------------
    struct SFieldMeta
    {
        float                    min     = 0.0f;   // min == max: unbounded drag
        float                    max     = 0.0f;
        float                    speed   = 0.01f;  // drag speed when unbounded
        const char*              tooltip = nullptr;
        std::vector<const char*> options;          // int fields: shown as a combo
        bool                     color   = false;  // SVector3 fields: colour picker instead of XYZ drag
    };

    // Override to give fields ranges, tooltips or combo labels.
    [[nodiscard]] virtual SFieldMeta getFieldMeta(const std::string& fieldName) const { return {}; }

    // ---- Access by field name (inspector, scripts) -------------------------
    // Typed C++ code should use the fields directly; these exist for code
    // that only knows the name at runtime. False / fallback if the field
    // doesn't exist or has a different type.
    template <typename T>
    bool setPropertyValue(const std::string& fieldName, const T& value)
    {
        if (auto* f = findField<T>(fieldName)) { f->set(value); return true; }
        return false;
    }

    template <typename T>
    T getPropertyValue(const std::string& fieldName, const T& fallback = T{}) const
    {
        if (auto* f = findField<T>(fieldName)) return f->get();
        return fallback;
    }

    template <typename T>
    Field<T>* findField(const std::string& fieldName) const
    {
        for (FieldBase* f : fields)
            if (f->name == fieldName) return dynamic_cast<Field<T>*>(f);
        return nullptr;
    }
};

#endif // POST_PROCESS_SETTINGS_H
