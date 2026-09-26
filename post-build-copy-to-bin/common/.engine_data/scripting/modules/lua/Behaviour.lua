-- behaviour.lua
-- Base class for all Lua script behaviours.

---@class Behaviour
Behaviour = {}
Behaviour.__index = Behaviour

-- Standard constructor / wrapping call
Behaviour.__call = function(cls, scriptTable)
    if scriptTable == nil then
        scriptTable = {}
    end
    -- Instantiates scriptTable using 'cls' (e.g. PlayerInput) as its metatable
    return setmetatable(scriptTable, cls)
end

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

--- Create a new subclass extending this class.
---@param t table|nil
---@return table
function Behaviour:extend(t)
    if type(t) == "table" then
        for k,v in pairs(t) do
            MLogger.info("EXTEND GOT: " .. tostring(k))
        end
    end
    
    local cls = t or {}
    cls.__index = cls
    setmetatable(cls, {
        __index = self,
        __call = function(class_tbl, instance_tbl)
            instance_tbl = instance_tbl or {}
            return setmetatable(instance_tbl, class_tbl)
        end
    })
    return cls
end

return Behaviour