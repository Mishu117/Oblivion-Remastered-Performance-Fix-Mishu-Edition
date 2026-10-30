-- SneakProbe: investigation tool for the "sneak attack for 1X damage" bow bug (not part of the shipped fix).
--
-- What the game does (Steam build 19115871; evidence in docs/research.md, "Bow sneak attacks at 1x"):
-- when an arrow lands, AVAmmunition's impact handler (OnBounce, or UVHitBoxComponent's OnOverlapTriggered
-- for the pawn hit box) asks the shooter for GetBaseSneakDamageMultiplierForCurrentWeapon() right then.
-- That reads AVPairedPawn.WeaponsPairingComponent.WeaponActor.WeaponTypeTag and looks the tag up in
-- UVOblivionRuntimeSettings' WeaponSneakDamageMultiplierDataTable; no WeaponActor (null, being destroyed)
-- or a tag without a row gives 1.0. The value is queued to the old engine, which multiplies the arrow
-- damage by it and shows "Sneak attack for <round(value)>X". So the multiplier is sampled at impact, not
-- at release, from whatever WeaponActor the shooter has then.
--
-- The first version relied on UFunction hooks; in a play session they never fired (the game calls those
-- functions natively, which UE4SS hooks do not see). This version watches every frame instead:
--   HEARTBEAT  every 5 s: wall clock, frame count, frames/s of this loop, the player's raw state, live arrows
--   MULT       GetBaseSneakDamageMultiplierForCurrentWeapon() is called every frame; every change is logged,
--              and every frame it differs from the settled value (held >= 60 frames) gets an "OFF" line
--   WEAPON     every change of WeaponForm / WeaponActor / WeaponTypeTag (null pointers show as "null")
--   ARROW      VAmmunition actors found by polling: NEW (with the state that frame), STOP (speed fell
--              below 50 units/s: it hit something; the state that frame is the one the impact read), GONE
--   RELEASE / IMPACT  the old UFunction hooks, kept in case some path does go through them
-- "SUSPECT" marks a STOP or IMPACT whose multiplier that frame is 1.00 (what the 1X message shows).
-- Log lines start with the wall-clock time (to line up with a video) and the frame number.
--
-- Dormant unless <runtime>sneak-probe-on.txt exists (tools\Install-UE4SS.ps1 deploys every mod in
-- mods\ue4ss, so this keeps benchmark runs untouched). Log: <runtime>sneak-probe.log (appended).
-- @RUNTIME@ is written into the deployed copy by tools\Install-UE4SS.ps1 (run\ in the repo).

local RUNTIME = "@RUNTIME@"
local ON_FILE = RUNTIME .. "sneak-probe-on.txt"
local LOG_FILE = RUNTIME .. "sneak-probe.log"

local HEARTBEAT_S = 5
local SETTLED_FRAMES = 60        -- a multiplier held this long is "the" value; others are logged per frame
local OFF_LINES_MAX = 600        -- per episode, so a long odd value cannot flood the log
local STOP_SPEED = 50            -- units/s: an arrow slower than this has hit something

do
    local f = io.open(ON_FILE, "r")
    if not f then print("[SneakProbe] dormant (no " .. ON_FILE .. ")\n"); return end
    f:close()
end

local frame = 0

local function log(msg)
    local f = io.open(LOG_FILE, "a")
    if f then
        local c = os.clock()
        f:write(string.format("%s.%03d f=%d %s\n", os.date("%H:%M:%S"), math.floor((c % 1) * 1000), frame, msg))
        f:close()
    end
end

-- Errors from code that runs every frame: the first one is logged, then at most one line a second.
local errAt = {}
local function LogError(where, err)
    local now = os.clock()
    if not errAt[where] or now - errAt[where] >= 1 then
        errAt[where] = now
        log("ERROR " .. where .. ": " .. (tostring(err):gsub("[\r\n]+", " | ")))
    end
end

-- ---------------------------------------------------------------- object access

local function Addr(o)
    if o == nil then return 0 end
    local ok, a = pcall(function() return o:GetAddress() end)
    return ok and a or 0
end

local function Hex(a) return a == 0 and "null" or string.format("0x%X", a) end

local function Name(o)
    if o == nil or Addr(o) == 0 then return "null" end
    local ok, valid = pcall(function() return o:IsValid() end)
    if not ok then return "?" end
    if not valid then return "invalid" end          -- flagged for destruction / unreachable
    local ok2, n = pcall(function() return o:GetFName():ToString() end)
    return ok2 and n or "?"
end

local function ClassName(o)
    local ok, n = pcall(function() return o:GetClass():GetFName():ToString() end)
    return ok and n or "?"
end

-- The local player's pawn: PlayerController with a Player, re-found when it goes away.
local cache = {}
local function Pawn()
    local pc = cache.pc
    if not (pc and pc:IsValid()) then
        pc = nil
        -- FindAllOf scans every object: while there is no player (menus, loading) at most once a second
        local now = os.clock()
        if cache.missAt and now - cache.missAt < 1.0 then return nil end
        cache.missAt = now
        for _, p in ipairs(FindAllOf("PlayerController") or {}) do
            if p:IsValid() then
                local ok, has = pcall(function() return p.Player and p.Player:IsValid() end)
                if ok and has then pc = p; break end
            end
        end
        cache.pc = pc
    end
    if not pc then return nil end
    local ok, pawn = pcall(function() return pc.Pawn end)
    if ok and pawn and Addr(pawn) ~= 0 and pawn:IsValid() then return pawn end
    return nil
end

-- The player's paired-weapon state: what GetBaseSneakDamageMultiplierForCurrentWeapon reads.
-- WeaponForm (+0x160) and WeaponActor (+0x168) are plain object pointers in UVWeaponsPairingComponent.
local function WeaponState(pawn)
    local s = { form = "?", actor = "?", actorAddr = 0, tag = "-", mult = "?" }
    if not pawn then s.form = "nopawn"; return s end
    local okC, comp = pcall(function() return pawn.WeaponsPairingComponent end)
    if not (okC and comp and Addr(comp) ~= 0) then s.form = "nocomp"; return s end
    local okF, form = pcall(function() return comp.WeaponForm end)
    if okF then s.form = Name(form) end
    local okA, actor = pcall(function() return comp.WeaponActor end)
    if okA then
        s.actorAddr = Addr(actor)
        s.actor = Name(actor)
        if s.actorAddr ~= 0 then
            s.actorClass = ClassName(actor)
            local okT, tag = pcall(function() return actor.WeaponTypeTag.TagName:ToString() end)
            s.tag = okT and tag or "?"
        end
    end
    local okM, m = pcall(function() return pawn:GetBaseSneakDamageMultiplierForCurrentWeapon() end)
    if okM and type(m) == "number" then s.mult = string.format("%.2f", m) end
    return s
end

local function StateText(s)
    return string.format("form=%s actor=%s%s@%s tag=%s mult=%s", s.form, s.actor,
        s.actorClass and ("(" .. s.actorClass .. ")") or "", Hex(s.actorAddr), s.tag, s.mult)
end

-- Sneaking (for context only; the game's own check is in the old engine): the first reader that works.
local sneakReaders = {
    { "pawn:IsSneaking()", function(p) return p:IsSneaking() end },
    { "OblivionActorStatePairingComponent:IsSneaking()", function(p) return p.OblivionActorStatePairingComponent:IsSneaking() end },
    { "PairedPawnMovementComponent.bWantsToSneak", function(p) return p.PairedPawnMovementComponent.bWantsToSneak end },
    { "PairedPawnMovementComponent:IsSneaking()", function(p) return p.PairedPawnMovementComponent:IsSneaking() end },
}
local sneakReader, sneakTried = nil, false
local function Sneaking(pawn)
    if not pawn then return "?" end
    if sneakReader then
        local ok, v = pcall(sneakReader[2], pawn)
        return ok and tostring(v) or "?"
    end
    if sneakTried then return "?" end
    sneakTried = true
    for _, r in ipairs(sneakReaders) do
        local ok, v = pcall(r[2], pawn)
        if ok and type(v) == "boolean" then
            sneakReader = r
            log("INFO sneaking read from " .. r[1])
            return tostring(v)
        end
    end
    log("INFO no sneaking reader works (sneak=? in arrow lines)")
    return "?"
end

-- ---------------------------------------------------------------- arrows (polled)

local arrows = {}               -- address -> { obj, name, frame, t, mine, stopped, s }
local liveArrows = 0

local function Shooter(arrow)
    local ok, who = pcall(function() return arrow:GetInstigator() end)
    if ok and who and Addr(who) ~= 0 then return who end
    ok, who = pcall(function() return arrow:GetOwner() end)
    if ok and who and Addr(who) ~= 0 then return who end
    return nil
end

local function Speed(arrow)
    local ok, v = pcall(function() return arrow:GetVelocity() end)
    if not (ok and v) then return nil end
    local ok2, sp = pcall(function() return math.sqrt(v.X * v.X + v.Y * v.Y + v.Z * v.Z) end)
    return ok2 and sp or nil
end

local function PollArrows(pawn, s)
    local seen = {}
    local pawnAddr = Addr(pawn)
    local list = FindAllOf("VAmmunition") or {}
    for _, a in ipairs(list) do
        local addr = Addr(a)
        if addr ~= 0 then
            seen[addr] = true
            local r = arrows[addr]
            if not r then
                local who = Shooter(a)
                local mine = who and pawnAddr ~= 0 and Addr(who) == pawnAddr
                r = { name = Name(a), frame = frame, t = os.clock(), mine = mine, s = s }
                arrows[addr] = r
                if mine or not who then
                    log(string.format("ARROW NEW %s shooter=%s %s sneak=%s", r.name,
                        mine and "player" or "unknown", StateText(s), Sneaking(pawn)))
                end
            end
            if not r.stopped and (r.mine or r.mine == nil) then
                local sp = Speed(a)
                if sp then
                    if sp >= STOP_SPEED then r.flew = true
                    elseif r.flew then
                        r.stopped = true
                        log(string.format("ARROW STOP%s %s after %d frames (%.3fs) %s sneak=%s | at_new actor=%s tag=%s mult=%s",
                            s.mult == "1.00" and " SUSPECT" or "", r.name, frame - r.frame, os.clock() - r.t,
                            StateText(s), Sneaking(pawn), Hex(r.s.actorAddr), r.s.tag, r.s.mult))
                    end
                end
            end
        end
    end
    liveArrows = 0
    for addr, r in pairs(arrows) do
        if seen[addr] then liveArrows = liveArrows + 1
        else
            if r.mine or r.mine == nil then
                log(string.format("ARROW GONE %s after %d frames%s mult=%s", r.name, frame - r.frame,
                    r.stopped and "" or " (no STOP seen)", s.mult))
            end
            arrows[addr] = nil
        end
    end
end

-- ---------------------------------------------------------------- hooks (kept; they did not fire in play)

local function OnHook(kind, arrow, extra)
    local pawn = Pawn()
    local s = WeaponState(pawn)
    log(string.format("%s%s arrow=%s %s sneak=%s%s", kind,
        (kind ~= "RELEASE" and s.mult == "1.00") and " SUSPECT" or "", Name(arrow), StateText(s), Sneaking(pawn), extra or ""))
end

local hooks = {
    { path = "/Script/Altar.VAmmunition:ShootAmunition", fn = function(ctx) OnHook("RELEASE", ctx:get()) end },
    { path = "/Script/Altar.VAmmunition:OnBounce", fn = function(ctx) OnHook("IMPACT", ctx:get(), " path=bounce") end },
    { path = "/Script/Altar.VHitBoxComponent:OnOverlapTriggered", fn = function(ctx)
        local comp = ctx:get()
        local ok, owner = pcall(function() return comp:GetOwner() end)
        if ok and owner and Addr(owner) ~= 0 and ClassName(owner):find("Ammunition") then
            OnHook("IMPACT", owner, " path=hitbox hitbox=" .. Name(comp))
        end
    end },
}

local function TryHooks()
    local pending = 0
    for _, h in ipairs(hooks) do
        if not h.done then
            local ok, err = pcall(RegisterHook, h.path, function(ctx, ...)
                local ok2, e = pcall(h.fn, ctx, ...)     -- never let an error reach UE4SS
                if not ok2 then LogError(h.path, e) end
            end)
            if ok then h.done = true; log("HOOKED " .. h.path)
            else
                h.tries = (h.tries or 0) + 1
                if h.tries == 1 or h.tries % 30 == 0 then log("WAIT " .. h.path .. ": " .. (tostring(err):gsub("[\r\n]+", " | "))) end
                pending = pending + 1
            end
        end
    end
    return pending
end

-- ---------------------------------------------------------------- per-frame watch

local lastWeapon = nil
local mult = { cur = nil, since = 0, settled = nil, offLines = 0 }
local nextHookTry, nextBeat, beatFrame = 0, 0, 0

local function WatchMult(s)
    local m = s.mult
    if m ~= mult.cur then
        log(string.format("MULT %s -> %s (held %d frames) %s", tostring(mult.cur), m, frame - mult.since, StateText(s)))
        mult.cur = m; mult.since = frame; mult.offLines = 0
    elseif mult.settled ~= m and frame - mult.since >= SETTLED_FRAMES then
        mult.settled = m
        log("MULT settled at " .. m)
    end
    if mult.settled and m ~= mult.settled and m ~= "?" and mult.offLines < OFF_LINES_MAX then
        mult.offLines = mult.offLines + 1
        log(string.format("MULT OFF %s (settled %s, frame %d of this value) %s", m, mult.settled, frame - mult.since + 1, StateText(s)))
    end
end

local function OnFrame()
    frame = frame + 1
    local now = os.clock()
    if now >= nextHookTry then
        nextHookTry = now + 1
        if TryHooks() == 0 then nextHookTry = math.huge end
    end
    local pawn = Pawn()
    local s = WeaponState(pawn)
    if now >= nextBeat then
        local fps = nextBeat > 0 and (frame - beatFrame) / (now - nextBeat + HEARTBEAT_S) or 0
        nextBeat = now + HEARTBEAT_S; beatFrame = frame
        log(string.format("HEARTBEAT loop=%.0f/s pawn=%s%s %s arrows=%d sneak=%s", fps,
            pawn and ClassName(pawn) or "none", pawn and ("@" .. Hex(Addr(pawn))) or "", StateText(s), liveArrows,
            Sneaking(pawn)))
    end
    if not pawn then return end
    local key = s.form .. "|" .. Hex(s.actorAddr) .. "|" .. s.tag
    if key ~= lastWeapon then
        lastWeapon = key
        log("WEAPON " .. StateText(s))
    end
    WatchMult(s)
    local ok, err = pcall(PollArrows, pawn, s)
    if not ok then LogError("arrows", err) end
end

log("READY SneakProbe v2 (log " .. LOG_FILE .. ")")
if type(LoopInGameThreadAfterFrames) == "function" then
    LoopInGameThreadAfterFrames(1, function()
        local ok, err = pcall(OnFrame)
        if not ok then LogError("frame", err) end
    end)
else
    -- No frame loop in this UE4SS build: a 20 ms async tick that hands the watch to the game thread
    -- (coarser: a multiplier dip shorter than a tick can be missed).
    log("WARN no LoopInGameThreadAfterFrames: watching every 20 ms instead of every frame")
    local pending = false
    LoopAsync(20, function()
        if not pending then
            pending = true
            ExecuteInGameThread(function()
                local ok, err = pcall(OnFrame)
                if not ok then LogError("frame", err) end
                pending = false
            end)
        end
        return false
    end)
end
