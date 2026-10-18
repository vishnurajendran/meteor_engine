-- behaviour.lua
-- Base class for all Lua script behaviours.
--
-- A script file defines a CLASS:
--
--   MODULE "game.camera"                   (optional — default meteor.game)
--
--   local CameraController = DEFINE_CLASS("CameraController", Behaviour) {
--       speed = 5,
--       move = function(self, x, y) ... end,
--   }
--
--   return CameraController
--
-- MODULE and DEFINE_CLASS are provided by the engine while the file loads
-- (one copy per script). The engine also reads them from the file text, so
-- names must be string literals. DEFINE_CLASS calls Base:extend below.
--
-- The engine compiles each file once and gives every entity its own
-- INSTANCE: a small table { __entity = <owner> } whose metatable is the
-- class. Fields written through self land on the instance; methods and
-- default values are read from the class.
--
-- Calling a class CASTS, the same way the C++ types do:
--
--   MAudioSource(entity)                  -- nil if not an audio source
--   CameraController(entt:getScript())    -- nil if not a CameraController
--
-- A subclass passes the cast of every class above it.

---@class Behaviour
Behaviour = {}
Behaviour.__index = Behaviour

--- Returns obj if it is an instance of cls or of a subclass, otherwise nil.
--- Never modifies obj.
local function castTo(cls, obj)
    if type(obj) ~= "table" then
        return nil
    end

    -- instance → its class → parent class → ... → Behaviour
    local current = getmetatable(obj)
    while current ~= nil do
        if rawequal(current, cls) then
            return obj
        end
        local meta = getmetatable(current)
        current = meta and rawget(meta, "__index") or nil
    end
    return nil
end

-- Behaviour(obj) casts to "any script".
setmetatable(Behaviour, { __call = castTo })

-- Default lifecycle stubs (no-op instead of erroring so unhandled callbacks safely degrade)
function Behaviour:onStart() end
function Behaviour:onTick(dt) end
function Behaviour:onFixedTick(dt) end
function Behaviour:onStop() end

--- Returns the owning MSpatialEntity.
---@return MSpatialEntity
function Behaviour:myEntity()
    return self.__entity
end

--- Create a new class extending this one. The table holds the class's
--- fields and methods. Calling the returned class casts — see above.
--- Scripts use DEFINE_CLASS, which calls this and makes the class importable.
---@param t table|nil
---@return table
function Behaviour:extend(t)
    local cls = t or {}
    cls.__index = cls
    setmetatable(cls, {
        __index = self,
        __call = castTo,
    })
    return cls
end

return Behaviour
