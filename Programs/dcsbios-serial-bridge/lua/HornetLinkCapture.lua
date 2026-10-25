-- Read-only broad native capture. See docs/NATIVE_DCS_VERIFICATION.md.
local ARG_MAX = 4095
local DISPLAY_MAX = 63
local ARG_INTERVAL = 0.1
local DETAIL_INTERVAL = 1
local CHUNK_BYTES = 1500
-- External candidates: installed reference plus launch bar 85 from v0.11.7.
local EXTERNAL_ARGS = {1, 4, 6, 8, 21, 22, 25, 85, 88, 89, 90, 190, 191, 192, 193, 212, 323}
local RADIO_DEVICES = {38, 39}
local APIs = {
    "LoGetVersionInfo", "LoGetSelfData", "LoGetMissionStartTime",
    "LoGetPlayerPlaneId", "LoGetPlayerUnitId", "LoGetInAir",
    "LoGetIndicatedAirSpeed", "LoGetTrueAirSpeed",
    "LoGetAltitudeAboveSeaLevel", "LoGetAltitudeAboveGroundLevel",
    "LoGetAngleOfAttack", "LoGetAngleOfSideSlip", "LoGetAccelerationUnits",
    "LoGetShakeAmplitude", "LoGetVerticalVelocity", "LoGetMachNumber",
    "LoGetADIPitchBankYaw", "LoGetMagneticYaw", "LoGetGlideDeviation",
    "LoGetSideDeviation", "LoGetSlipBallPosition", "LoGetBasicAtmospherePressure",
    "LoGetRadarAltimeter", "LoGetControlPanel_HSI", "LoGetEngineInfo",
    "LoGetRoute", "LoGetNavigationInfo", "LoGetMCPState",
    "LoGetCameraPosition", "LoGetSightingSystemInfo", "LoGetTWSInfo",
    "LoGetPayloadInfo", "LoGetMechInfo", "LoGetRadioBeaconsStatus",
    "LoGetVectorVelocity", "LoGetAngularVelocity", "LoGetVectorWindVelocity",
    "LoGetHeightWithObjects",
}

local function quote(value)
    return '"' .. value:gsub('[%z\1-\31\128-\255\\"]', function(ch)
        if ch == '"' then return '\\"' end
        if ch == "\\" then return "\\\\" end
        return string.format("\\u%04x", ch:byte())
    end) .. '"'
end

local function json(value, parents, depth)
    local kind = type(value)
    if kind == "nil" then return "null" end
    if kind == "boolean" then return tostring(value) end
    if kind == "number" then
        if value ~= value or math.abs(value) == math.huge then error("Non-finite number") end
        return string.format("%.17g", value)
    end
    if kind == "string" then return quote(value) end
    if kind ~= "table" then error("Unsupported native value: " .. kind) end
    parents, depth = parents or {}, depth or 0
    if parents[value] or depth > 32 then error("Cyclic or excessively nested native value") end
    parents[value] = true
    local keys, fields, names = {}, {}, {}
    for key in pairs(value) do
        if type(key) ~= "number" and type(key) ~= "string" then error("Unsupported table key") end
        local name = tostring(key)
        if names[name] then error("Colliding native table keys: " .. name) end
        names[name] = key
        keys[#keys + 1] = name
    end
    table.sort(keys)
    for _, name in ipairs(keys) do
        fields[#fields + 1] = quote(name) .. ":" .. json(value[names[name]], parents, depth + 1)
    end
    parents[value] = nil
    return "{" .. table.concat(fields, ",") .. "}"
end

local session, sequence, segment, starts = nil, 0, 0, 0
local active, identity, previousTime, nextArgs, nextDetails, nextHeartbeat
local last, statuses, counts = {}, {}, {}
local failed = false

local function emit(kind, key, value, time)
    sequence = sequence + 1
    local payload = json({
        schema = 1, session = session, seq = sequence, segment = segment,
        kind = kind, key = key, value = value, time = time,
    })
    local parts = math.max(1, math.ceil(#payload / CHUNK_BYTES))
    for part = 1, parts do
        log.write("HornetLinkCapture", log.INFO, string.format("HLNC %s %d %d/%d %s",
            session, sequence, part, parts,
            payload:sub((part - 1) * CHUNK_BYTES + 1, part * CHUNK_BYTES)))
    end
end

local function status(key, message, time)
    if statuses[key] ~= message then
        emit("status", key, message, time)
        statuses[key] = message
    end
end

local function changed(kind, key, value, time)
    local encoded = json(value)
    local cacheKey = kind .. ":" .. key
    if last[cacheKey] ~= encoded then
        emit(kind, key, value, time)
        last[cacheKey] = encoded
        counts.changed = counts.changed + 1
    else
        counts.repeated = counts.repeated + 1
    end
end

local function pack(...)
    local result = {n = select("#", ...)}
    for index = 1, result.n do
        local value = select(index, ...)
        -- Preserve nil returns explicitly without confusing them with absent samples.
        result[tostring(index)] = {present = value ~= nil, value = value}
    end
    return result
end

local function finite(value)
    return type(value) == "number" and value == value and math.abs(value) ~= math.huge
end

local function readRadios(time)
    for _, id in ipairs(RADIO_DEVICES) do
        local key = tostring(id)
        local ok, value = pcall(function()
            local device = GetDevice(id)
            if not device then error("Radio device unavailable") end
            return device:get_frequency()
        end)
        if ok and finite(value) and value >= 0 then
            changed("radio_frequency", key, value, time)
            status("radio:" .. key, "ok", time)
        else
            last["radio_frequency:" .. key] = nil
            status("radio:" .. key,
                ok and "invalid frequency (expected finite non-negative Hz)" or "error: " .. tostring(value), time)
            counts.errors = counts.errors + 1
        end
    end
    counts.radio_scans = counts.radio_scans + 1
end

local function readExternal(time)
    local delta, errors = {}, {}
    for _, arg in ipairs(EXTERNAL_ARGS) do
        local key = tostring(arg)
        local ok, value = pcall(function() return _G.LoGetAircraftDrawArgumentValue(arg) end)
        if ok and finite(value) then
            if last["external:" .. key] ~= value then
                delta[key] = value
                last["external:" .. key] = value
                counts.changed = counts.changed + 1
            else
                counts.repeated = counts.repeated + 1
            end
        else
            errors[key] = ok and "non-finite or non-numeric result" or tostring(value)
            last["external:" .. key] = nil
            counts.errors = counts.errors + 1
        end
    end
    if next(delta) then emit("external_arguments", "own-aircraft", delta, time) end
    changed("external_argument_errors", "own-aircraft", errors, time)
    counts.external_scans = counts.external_scans + 1
end

local function readDetails(time)
    readRadios(time)
    for display = 0, DISPLAY_MAX do
        local ok, value = pcall(list_indication, display)
        local key = "display:" .. display
        if ok and type(value) == "string" then
            changed("indication", tostring(display), value, time)
            status(key, value == "" and "empty" or "ok", time)
        else
            status(key, ok and "unavailable (nil or non-string)" or "error: " .. tostring(value), time)
            counts.errors = counts.errors + 1
        end
    end
    for _, name in ipairs(APIs) do
        local fn = _G[name]
        if type(fn) ~= "function" then
            status(name, "API unavailable", time)
        else
            local ok, value = pcall(function() return json(pack(fn())) end)
            if ok then
                -- A serialized result is retained verbatim, including multiple/nil returns.
                changed("telemetry", name, value, time)
                status(name, "ok", time)
            else
                status(name, "error: " .. tostring(value), time)
                counts.errors = counts.errors + 1
            end
        end
    end
end

local function readArgs(time)
    local device = GetDevice(0)
    if not device then
        status("arguments", "cockpit device 0 unavailable", time)
        return
    end
    status("arguments", "ok", time)
    local delta, errors = {}, {}
    for arg = 0, ARG_MAX do
        local ok, value = pcall(device.get_argument_value, device, arg)
        local key = tostring(arg)
        if ok and type(value) == "number" and value == value and math.abs(value) ~= math.huge then
            if last["arg:" .. key] ~= value then
                delta[key] = value
                last["arg:" .. key] = value
                counts.changed = counts.changed + 1
            else
                counts.repeated = counts.repeated + 1
            end
        else
            errors[key] = ok and "non-finite or non-numeric result" or tostring(value)
            counts.errors = counts.errors + 1
            -- A later recovery must produce a fresh sample, even if unchanged.
            last["arg:" .. key] = nil
        end
    end
    if next(delta) then emit("arguments", "device:0", delta, time) end
    changed("argument_errors", "device:0", errors, time)
    counts.argument_scans = counts.argument_scans + 1
end

local function reset()
    last, statuses = {}, {}
    nextArgs, nextDetails, nextHeartbeat = 0, 0, 0
    counts = {
        changed = 0, repeated = 0, errors = 0, argument_scans = 0, detail_scans = 0,
        external_scans = 0, radio_scans = 0,
    }
end

local function start()
    starts = starts + 1
    session = os.date("!%Y-%m-%dT%H:%M:%SZ") .. "-" .. starts
    sequence, segment, active, identity, previousTime = 0, 0, false, nil, nil
    reset()
    emit("start", "capture", {
        aircraft = "FA-18C_hornet", argument_min = 0, argument_max = ARG_MAX,
        display_min = 0, display_max = DISPLAY_MAX, argument_interval = ARG_INTERVAL,
        detail_interval = DETAIL_INTERVAL, telemetry_apis = APIs,
        external_arguments = EXTERNAL_ARGS, external_interval = ARG_INTERVAL,
        external_api = "LoGetAircraftDrawArgumentValue",
        radio_devices = RADIO_DEVICES, radio_interval = DETAIL_INTERVAL, radio_units = "Hz",
        strings = "byte-preserving JSON: bytes >=128 mapped to U+0080..U+00FF",
    })
end

local function sample()
    if not session then return end
    local self = LoGetSelfData()
    if not self or self.Name ~= "FA-18C_hornet" then
        if active then emit("inactive", "capture", counts, previousTime) end
        active, identity, previousTime = false, nil, nil
        return
    end
    local time = LoGetModelTime()
    if type(time) ~= "number" or time ~= time or math.abs(time) == math.huge then
        error("Invalid DCS model time")
    end
    local currentIdentity = tostring(self.UnitName) .. ":" .. tostring(self.Name)
    if not active or identity ~= currentIdentity or (previousTime and time < previousTime) then
        if active then emit("inactive", "capture", counts, previousTime) end
        segment = segment + 1
        reset()
        active, identity = true, currentIdentity
        emit("active", "capture", self, time)
    end
    previousTime = time
    if time >= nextArgs then
        readArgs(time)
        readExternal(time)
        nextArgs = time + ARG_INTERVAL
    end
    if time >= nextDetails then
        readDetails(time)
        counts.detail_scans = counts.detail_scans + 1
        nextDetails = time + DETAIL_INTERVAL
    end
    if time >= nextHeartbeat then
        emit("heartbeat", "capture", counts, time)
        nextHeartbeat = time + 5
    end
end

local function guarded(fn)
    if failed then return end
    local ok, err = pcall(fn)
    if not ok then
        failed = true
        log.write("HornetLinkCapture", log.ERROR, tostring(err))
        emit("fatal", "capture", tostring(err), previousTime)
    end
end

local oldStart, oldFrame, oldStop = LuaExportStart, LuaExportAfterNextFrame, LuaExportStop
function LuaExportStart()
    if oldStart then oldStart() end
    failed = false
    guarded(start)
end
function LuaExportAfterNextFrame()
    if oldFrame then oldFrame() end
    guarded(sample)
end
function LuaExportStop()
    if oldStop then oldStop() end
    if session then emit("stop", "capture", counts, previousTime) end
    session, active = nil, false
end
