-- WidgetBehaviour.lua
-- Base class for UI widget behaviours. Written like any other script:
--
--   local Hud = CLASS("Hud", WidgetBehaviour) {
--       _healthLabel = nil,
--
--       onCreate = function(self)                 -- document loaded, not yet on screen
--           self._healthLabel = self.widget:findElement("health")
--       end,
--
--       setHealth = function(self, value)         -- called by gameplay via getBehaviour()
--           self._healthLabel:setText(tostring(value))
--       end,
--
--       onPauseClicked = function(self, event)    -- from onclick="onPauseClicked" in the .rml
--           MApplication.getAppInstance():pause(true)
--       end,
--   }
--
--   return Hud
--
-- A widget's .muia names the script as its behaviour. Each live widget gets
-- its own INSTANCE: { widget = <MUIWidget> }, the class as its metatable, the
-- same way entity scripts get { __entity = <entity> }. Two Huds have
-- separate state.
--
-- Hooks - the engine only calls the ones a script defines:
--
--   onCreate(self)             the widget's elements exist; nested widgets'
--                              behaviours already ran their onCreate. Runs
--                              again when the .rml is hot-reloaded - find
--                              elements here, not in field defaults.
--   onAddToViewport(self)      each time it is added to the viewport
--   onRemove(self)             it left the viewport: removed, destroyed, or
--                              Play stopped
--   onWidgetTick(self, dt)     every frame while in the viewport, after every
--                              entity's onLateTick. While paused only if
--                              tickWhenPaused (the .muia's setting, or
--                              self.widget:setTickWhenPaused). dt is real time.
--
-- Events: an on<event> attribute in the widget's .rml names a method here,
-- called with an SUIEvent (type, targetId, currentId, mouseX, mouseY, key,
-- value, stopPropagation()). onclick on a nested <widget> tag goes to the
-- widget whose .rml it is written in.
--
-- A WidgetBehaviour can't run on an entity, and an entity Behaviour can't be
-- a widget's behaviour: each is refused with an error.

---@class WidgetBehaviour : Behaviour
---@field widget MUIWidget The widget this behaviour belongs to
---@field onCreate? fun(self: WidgetBehaviour)
---@field onAddToViewport? fun(self: WidgetBehaviour)
---@field onRemove? fun(self: WidgetBehaviour)
---@field onWidgetTick? fun(self: WidgetBehaviour, dt: number)
WidgetBehaviour = Behaviour:extend({})

--- Returns the owning MUIWidget handle.
---@return MUIWidget
function WidgetBehaviour:myWidget()
    return self.__widget
end

-- Hand-bound parts of the UI API (ui_bindings.h), declared here so the
-- language server knows them.

---@class MUIWidget
---@field create fun(asset: string|MAssetHandle): MUIWidget Creates a widget from a .muia ("guid:..." or a path); add it with addToViewport
---@field getBehaviour fun(self: MUIWidget): WidgetBehaviour|nil The widget's behaviour instance, or nil

return WidgetBehaviour
