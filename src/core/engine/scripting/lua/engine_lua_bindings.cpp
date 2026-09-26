#include "engine_lua_bindings.h"
#include "binding_registry.generated.h"
#include "core/engine/scripting/bindings/log_bindings.h"
#include "core/engine/scripting/bindings/math_bindings.h"
#include "script_access_bingings.h"

void MLuaEngineBindings::BindEngineAPI(sol::state& vm)
{
    // Math types first (SVector3, SQuaternion, etc.)
    MeteorBindings::registerMathBindings(vm);

    // Logger (static utility)
    MeteorBindings::registerLoggerModule(vm);

    // All generated class bindings (MSpatialEntity, MScene, etc.)
    MeteorBindings::registerAllBindings(vm);

    // Script access (getScript/hasScript on MSpatialEntity)
    MeteorBindings::registerScriptAccessBindings(vm);
}