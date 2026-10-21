-- MMath.lua
-- Scalar math helpers for scripts. Engine module: loaded into every script
-- context at start-up, so MMath is a global - no require() needed.
--
-- Vector and quaternion maths live on the types themselves (native, GLM):
--   SVector3.distance(a, b), v:normalized(), SVector3.moveTowards(...),
--   SQuaternion.lookRotation(dir), q * v, ...
-- This module covers plain numbers, plus smoothing helpers that also accept
-- vectors where noted.
--
-- Angles are in DEGREES, matching the rest of the Lua API.
--
-- Frame-rate independence: use MMath.damp (or smoothDamp) for "chase a
-- target" smoothing. lerp(a, b, k * dt) looks smooth but behaves
-- differently at different frame rates.

---@class MMath
MMath = {}

MMath.PI       = math.pi
MMath.TAU      = math.pi * 2
MMath.DEG2RAD  = math.pi / 180
MMath.RAD2DEG  = 180 / math.pi
MMath.EPSILON  = 1e-6
MMath.INFINITY = math.huge

-- ---------------------------------------------------------------------------
-- Clamping and ranges
-- ---------------------------------------------------------------------------

--- Clamps value to [min, max].
---@param value number
---@param min number
---@param max number
---@return number
function MMath.clamp(value, min, max)
    if value < min then return min end
    if value > max then return max end
    return value
end

--- Clamps value to [0, 1].
---@param value number
---@return number
function MMath.clamp01(value)
    if value < 0 then return 0 end
    if value > 1 then return 1 end
    return value
end

--- -1, 0 or 1.
---@param value number
---@return number
function MMath.sign(value)
    if value > 0 then return 1 end
    if value < 0 then return -1 end
    return 0
end

--- True when a and b are within epsilon of each other (default 1e-6,
--- scaled up for large values).
---@param a number
---@param b number
---@param epsilon number|nil
---@return boolean
function MMath.approximately(a, b, epsilon)
    epsilon = epsilon or MMath.EPSILON
    return math.abs(a - b) <= epsilon * math.max(1, math.abs(a), math.abs(b))
end

--- Wraps value into [0, length). repeat_(370, 360) == 10, repeat_(-10, 360) == 350.
---@param value number
---@param length number
---@return number
function MMath.repeat_(value, length)
    return value - math.floor(value / length) * length
end

--- Bounces value back and forth between 0 and length.
---@param value number
---@param length number
---@return number
function MMath.pingPong(value, length)
    local t = MMath.repeat_(value, length * 2)
    return length - math.abs(t - length)
end

--- Rounds to the nearest multiple of step (default 1).
---@param value number
---@param step number|nil
---@return number
function MMath.round(value, step)
    step = step or 1
    return math.floor(value / step + 0.5) * step
end

-- ---------------------------------------------------------------------------
-- Interpolation
-- ---------------------------------------------------------------------------

--- Linear interpolation, t clamped to [0, 1].
---@param a number
---@param b number
---@param t number
---@return number
function MMath.lerp(a, b, t)
    return a + (b - a) * MMath.clamp01(t)
end

--- Linear interpolation without clamping t.
---@param a number
---@param b number
---@param t number
---@return number
function MMath.lerpUnclamped(a, b, t)
    return a + (b - a) * t
end

--- Where value sits between a and b, as 0..1 (clamped). 0 when a == b.
---@param a number
---@param b number
---@param value number
---@return number
function MMath.inverseLerp(a, b, value)
    if a == b then return 0 end
    return MMath.clamp01((value - a) / (b - a))
end

--- Maps value from [inMin, inMax] to [outMin, outMax] (clamped).
---@param value number
---@param inMin number
---@param inMax number
---@param outMin number
---@param outMax number
---@return number
function MMath.remap(value, inMin, inMax, outMin, outMax)
    return MMath.lerpUnclamped(outMin, outMax, MMath.inverseLerp(inMin, inMax, value))
end

--- Smooth 0..1 ramp as value goes from edge0 to edge1 (Hermite, GLSL smoothstep).
---@param edge0 number
---@param edge1 number
---@param value number
---@return number
function MMath.smoothstep(edge0, edge1, value)
    local t = MMath.inverseLerp(edge0, edge1, value)
    return t * t * (3 - 2 * t)
end

--- Like smoothstep with a flatter start and end (Perlin's smootherstep).
---@param edge0 number
---@param edge1 number
---@param value number
---@return number
function MMath.smootherstep(edge0, edge1, value)
    local t = MMath.inverseLerp(edge0, edge1, value)
    return t * t * t * (t * (t * 6 - 15) + 10)
end

--- Moves current toward target by at most maxDelta, never overshooting.
---@param current number
---@param target number
---@param maxDelta number
---@return number
function MMath.moveTowards(current, target, maxDelta)
    if math.abs(target - current) <= maxDelta then
        return target
    end
    return current + MMath.sign(target - current) * maxDelta
end

-- ---------------------------------------------------------------------------
-- Angles (degrees)
-- ---------------------------------------------------------------------------

--- Shortest signed difference from current to target, in [-180, 180].
---@param current number
---@param target number
---@return number
function MMath.deltaAngle(current, target)
    local delta = MMath.repeat_(target - current, 360)
    if delta > 180 then delta = delta - 360 end
    return delta
end

--- Wraps an angle into [-180, 180).
---@param degrees number
---@return number
function MMath.wrapAngle(degrees)
    return MMath.repeat_(degrees + 180, 360) - 180
end

--- Lerps between angles the short way round (t clamped).
---@param a number
---@param b number
---@param t number
---@return number
function MMath.lerpAngle(a, b, t)
    return a + MMath.deltaAngle(a, b) * MMath.clamp01(t)
end

--- moveTowards for angles: takes the short way round.
---@param current number
---@param target number
---@param maxDelta number
---@return number
function MMath.moveTowardsAngle(current, target, maxDelta)
    local delta = MMath.deltaAngle(current, target)
    if math.abs(delta) <= maxDelta then
        return current + delta
    end
    return current + MMath.sign(delta) * maxDelta
end

-- ---------------------------------------------------------------------------
-- Frame-rate independent smoothing
-- ---------------------------------------------------------------------------

--- Fraction to move this frame for exponential smoothing at `rate` per
--- second: 1 - e^(-rate * dt). Use as the t of any lerp/slerp.
---@param rate number
---@param dt number
---@return number
function MMath.dampFactor(rate, dt)
    return 1 - math.exp(-rate * dt)
end

--- Exponentially eases current toward target. Frame-rate independent:
--- higher rate = snappier. Works for numbers, SVector2/3/4 and SColor-free
--- types that support + - and * number. (Use SQuaternion.slerp with
--- MMath.dampFactor for rotations.)
---@generic T
---@param current T
---@param target T
---@param rate number
---@param dt number
---@return T
function MMath.damp(current, target, rate, dt)
    return current + (target - current) * MMath.dampFactor(rate, dt)
end

--- Critically damped spring toward target (Game Programming Gems 4).
--- Returns the new value and the new velocity; keep the velocity between
--- calls, starting at 0. smoothTime is roughly the time to reach the target.
---
---   self._x, self._xVel = MMath.smoothDamp(self._x, goal, self._xVel, 0.3, dt)
---
---@param current number
---@param target number
---@param velocity number
---@param smoothTime number
---@param dt number
---@param maxSpeed number|nil
---@return number value
---@return number velocity
function MMath.smoothDamp(current, target, velocity, smoothTime, dt, maxSpeed)
    smoothTime = math.max(0.0001, smoothTime)
    local omega = 2 / smoothTime
    local x = omega * dt
    local decay = 1 / (1 + x + 0.48 * x * x + 0.235 * x * x * x)

    local change = current - target
    local originalTarget = target
    if maxSpeed ~= nil then
        local maxChange = maxSpeed * smoothTime
        change = MMath.clamp(change, -maxChange, maxChange)
    end
    target = current - change

    local temp = (velocity + omega * change) * dt
    velocity = (velocity - omega * temp) * decay
    local result = target + (change + temp) * decay

    -- Don't overshoot the original target.
    if (originalTarget - current > 0) == (result > originalTarget) then
        result = originalTarget
        velocity = (result - originalTarget) / math.max(dt, MMath.EPSILON)
    end
    return result, velocity
end

--- smoothDamp for SVector3 (camera follow and the like). Same contract:
--- returns the new position and velocity; start velocity at SVector3.zero().
---@param current SVector3
---@param target SVector3
---@param velocity SVector3
---@param smoothTime number
---@param dt number
---@param maxSpeed number|nil
---@return SVector3 value
---@return SVector3 velocity
function MMath.smoothDampVector(current, target, velocity, smoothTime, dt, maxSpeed)
    smoothTime = math.max(0.0001, smoothTime)
    local omega = 2 / smoothTime
    local x = omega * dt
    local decay = 1 / (1 + x + 0.48 * x * x + 0.235 * x * x * x)

    local change = current - target
    local originalTarget = target
    if maxSpeed ~= nil then
        change = change:clampLength(maxSpeed * smoothTime)
    end
    target = current - change

    local temp = (velocity + change * omega) * dt
    velocity = (velocity - temp * omega) * decay
    local result = target + (change + temp) * decay

    -- Don't overshoot the original target.
    if SVector3.dot(originalTarget - current, result - originalTarget) > 0 then
        result = originalTarget
        velocity = (result - originalTarget) / math.max(dt, MMath.EPSILON)
    end
    return result, velocity
end

return MMath
