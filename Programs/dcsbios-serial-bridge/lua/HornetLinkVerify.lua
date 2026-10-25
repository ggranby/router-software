-- Read-only native DCS evidence capture; see docs/NATIVE_DCS_VERIFICATION.md.
-- Install separately from HornetLinkNative.lua. No sockets or input commands.
local arguments = {44, 45, 46, 47, 48, 49, 50, 458, 459}
local session
local last, active, nextFull = {}, false, 0
local failed = false
local starts = 0

local function writeRow(time, event, arg, value)
    log.write("HornetLinkVerify", log.INFO, string.format("CSV %s,%s,%s,%s,%s",
        session, time, event, arg or "", value or ""))
end

local function guarded(fn)
    if failed then return end
    local ok, err = pcall(fn)
    if not ok then
        failed = true
        log.write("HornetLinkVerify", log.ERROR, tostring(err))
    end
end

local function start()
    starts = starts + 1
    session = os.date("!%Y-%m-%dT%H:%M:%SZ") .. "-" .. starts
    last, active, nextFull = {}, false, 0
    writeRow("", "start")
    log.write("HornetLinkVerify", log.INFO, "Read-only Master Arm capture in dcs.log; CSV payloads follow")
end

local function sample()
    if not session then return end
    local self = LoGetSelfData()
    if not self or self.Name ~= "FA-18C_hornet" then
        if active then
            writeRow("", "inactive")
        end
        last, active, nextFull = {}, false, 0
        return
    end
    local time = LoGetModelTime()
    if type(time) ~= "number" or time ~= time or math.abs(time) == math.huge then
        error("Invalid DCS model time")
    end
    local device = GetDevice(0)
    if not device then error("DCS cockpit device 0 is unavailable") end
    local full = not active or time >= nextFull or time < nextFull - 1
    if not active then writeRow(time, "active") end
    active = true
    for _, arg in ipairs(arguments) do
        local value = device:get_argument_value(arg)
        if type(value) ~= "number" or value ~= value or math.abs(value) == math.huge then
            error("Invalid DCS argument value: " .. arg)
        end
        if full or last[arg] ~= value then
            writeRow(string.format("%.6f", time), full and "full" or "change",
                arg, string.format("%.17g", value))
            last[arg] = value
        end
    end
    if full then
        nextFull = time + 1
    end
end

local function stop()
    if not session then return end
    writeRow("", "stop")
    session = nil
end

local previousStart = LuaExportStart
local previousFrame = LuaExportAfterNextFrame
local previousStop = LuaExportStop

function LuaExportStart()
    if previousStart then previousStart() end
    failed = false
    guarded(start)
end

function LuaExportAfterNextFrame()
    if previousFrame then previousFrame() end
    guarded(sample)
end

function LuaExportStop()
    if previousStop then previousStop() end
    guarded(stop)
end
