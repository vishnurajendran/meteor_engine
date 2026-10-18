//
// engine_domain.cpp
//

#include "engine_domain.h"

#include "core/engine/scripting/lua/lua_script_context.h"
#include "core/utils/sstring.h"

MEngineDomain::MEngineDomain(uint32_t index)
    : index(index),
      scriptContext(std::make_unique<MLuaScriptContext>(SString::format("DOMAIN_{0}", SString::fromInt(static_cast<int>(index)))))
{
}

// Defined here so the unique_ptr can delete the context, which this
// header only forward-declares.
MEngineDomain::~MEngineDomain() = default;
