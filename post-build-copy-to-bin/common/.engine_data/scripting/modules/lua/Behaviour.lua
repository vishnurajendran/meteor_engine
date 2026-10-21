-- behaviour.lua
-- Base class for all Lua script behaviours.
--
-- A script file defines a CLASS:
--
--   MODULE "game.camera"                   (optional - default meteor.game)
--
--   local CameraController = CLASS("CameraController", Behaviour) {
--       speed = 5,
--       move = function(self, x, y) ... end,
--   }
--
--   return CameraController
--
-- MODULE and CLASS are provided by the engine while the file loads
-- (one copy per script). The engine also reads them from the file text, so
-- names must be string literals. CLASS calls Base:extend below.
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
--
-- Fields whose default is a boolean, number or string show in the
-- inspector's Behaviour Tuning section, where each entity can override them.
-- Start a name with '_' to keep it private (not shown, not tunable).

-- Physics messages: a script on an entity with a collision body may define
-- any of these, and the engine calls them on contact. They are deliberately
-- NOT defined here - the engine only calls the ones a script defines.
-- `data.otherEntity` is what was hit (cast it, e.g. MCollisionBodyEntity(...)).
--
--   onCollisionEnter = function(self, data) ... end,   -- SCollisionData
--   onTriggerEnter   = function(self, data) ... end,   -- SOverlapData
--
-- Stay variants run every physics step while touching; Exit when it ends
-- (for onCollisionExit only data.otherEntity is filled in).
--
-- They only run when the collision body has 'Receive Collision Events' /
-- 'Receive Trigger Events' turned on (off by default, for performance) - in
-- the inspector, or from a script: MCollisionBodyEntity(e).receiveTriggerEvents = true

---@class Behaviour
---@field onCollisionEnter? fun(self: Behaviour, data: SCollisionData)
---@field onCollisionStay? fun(self: Behaviour, data: SCollisionData)
---@field onCollisionExit? fun(self: Behaviour, data: SCollisionData)
---@field onTriggerEnter? fun(self: Behaviour, data: SOverlapData)
---@field onTriggerStay? fun(self: Behaviour, data: SOverlapData)
---@field onTriggerExit? fun(self: Behaviour, data: SOverlapData)
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
function Behaviour:onLateTick(dt) end
function Behaviour:onStop() end

--- Returns the owning MSpatialEntity.
---@return MSpatialEntity
function Behaviour:myEntity()
    return self.__entity
end

--- Create a new class extending this one. The table holds the class's
--- fields and methods. Calling the returned class casts - see above.
--- Scripts use CLASS, which calls this and makes the class importable.
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
