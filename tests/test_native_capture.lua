-- Run from repository root with Lua 5.1; optional first argument saves a mock log.
local lines, errors = {}, {}
local time, aircraft, unit = 0, "FA-18C_hornet", "test-unit"
local value, reads, chains, detailReads = 0, 0, 0, 0
local brokenArg = false
local externalValue, radioValue = 0.75, 251000000
local externalReads, radioReads = 0, 0
local extendedFailure = false
local externalIDs = {[1]=true, [4]=true, [6]=true, [8]=true, [21]=true, [22]=true,
    [25]=true, [85]=true, [88]=true, [89]=true, [90]=true, [190]=true, [191]=true,
    [192]=true, [193]=true, [212]=true, [323]=true}
local cyclic = {}
cyclic.self = cyclic
local function chain() chains = chains + 1 end
local env = setmetatable({
    io = {},
    log = {INFO = 1, ERROR = 2, write = function(_, level, message)
        if level == 2 then errors[#errors + 1] = message end
        lines[#lines + 1] = "TEST INFO HornetLinkCapture (Main): " .. message
    end},
    LoGetSelfData = function()
        if aircraft then return {Name = aircraft, UnitName = unit} end
    end,
    LoGetModelTime = function() return time end,
    LoGetVersionInfo = function() return {ProductVersion = {2, 9, 30, 28738}} end,
    LoGetADIPitchBankYaw = function() return 0.1, nil, -0.3 end,
    LoGetEngineInfo = function() return {RPM = {left = 70, right = 71}, flag = false} end,
    LoGetRoute = function() return cyclic end,
    LoGetMCPState = function() error("restricted") end,
    LoGetAircraftDrawArgumentValue = function(id)
        assert(externalIDs[id], "Unexpected external slot")
        externalReads = externalReads + 1
        if extendedFailure and id == 25 then error("external read restricted") end
        return (id == 25 or id == 85) and externalValue or 0
    end,
    list_indication = function(id)
        detailReads = detailReads + 1
        if id == 0 then return "" end
        if id == 6 then
            return "------\nUFC_ScratchPadNumberDisplay\n" .. value .. "\n------\nraw\n" ..
                string.rep("quoted \"line\\", 500) .. "\n\195\169\n"
        end
        if id == 5 then return "------\ntxt_RPM_L\n070\n" end
        if id == 63 then error("no such display") end
        return nil
    end,
    GetDevice = function(id)
        assert(id == 0 or id == 38 or id == 39, "Unexpected device read")
        if id ~= 0 then
            return {get_frequency = function()
                radioReads = radioReads + 1
                if extendedFailure and id == 38 then return 0 / 0 end
                return radioValue + (id - 38) * 1000000
            end}
        end
        return {get_argument_value = function(_, argument)
            assert(argument >= 0 and argument <= 4095)
            reads = reads + 1
            if argument == 9 and brokenArg then error("transient argument failure") end
            return argument == 49 and value or 0
        end}
    end,
    LuaExportStart = chain, LuaExportAfterNextFrame = chain, LuaExportStop = chain,
}, {__index = _G})
env._G = env
local chunk = assert(loadfile("Programs/dcsbios-serial-bridge/lua/HornetLinkCapture.lua"))
setfenv(chunk, env)()
env.LuaExportStart()
env.LuaExportAfterNextFrame()
assert(reads == 4096 and detailReads == 64)
assert(externalReads == 17 and radioReads == 2, "External/radio probes must run at declared rates")
local count = #lines
time = 0.05
env.LuaExportAfterNextFrame()
assert(#lines == count and reads == 4096, "Sampling must be throttled")
assert(externalReads == 17 and radioReads == 2)
time = 0.2
env.LuaExportAfterNextFrame()
assert(reads == 8192 and #lines == count, "Repeated values must be suppressed")
assert(externalReads == 34 and radioReads == 2)
time, value = 0.4, 0.125
env.LuaExportAfterNextFrame()
assert(lines[#lines]:find('"49":0.125', 1, true), "Fractional raw values must be preserved")
time, value, brokenArg, extendedFailure = 1.5, 1, true, true
env.LuaExportAfterNextFrame()
time, brokenArg, extendedFailure = 2.6, false, false
env.LuaExportAfterNextFrame()
local recovered = table.concat(lines, "\n")
assert(recovered:find('"9":0', 1, true), "Recovered arguments need fresh samples")
local _, externalBaselines = recovered:gsub('"25":0.75', "")
assert(externalBaselines == 2, "External recovery must re-emit unchanged data")
local _, frequencyBaselines = recovered:gsub('"value":251000000', "")
assert(frequencyBaselines == 2, "Radio recovery must re-emit unchanged frequency")
assert(recovered:find("external read restricted", 1, true))
assert(recovered:find("invalid frequency", 1, true))
local extendedBefore = externalReads + radioReads
time, aircraft = 2.7, "Other"
env.LuaExportAfterNextFrame()
local before = reads
time = 3
env.LuaExportAfterNextFrame()
assert(reads == before, "Other aircraft must not be scanned")
assert(externalReads + radioReads == extendedBefore, "Other aircraft must not produce external/radio reads")
time, aircraft = 4, "FA-18C_hornet"
env.LuaExportAfterNextFrame()
time = 0
env.LuaExportAfterNextFrame()
time, unit = 0.5, "respawn"
env.LuaExportAfterNextFrame()
time, externalValue, radioValue = 6, 1, 252000000
env.LuaExportAfterNextFrame()
env.LuaExportStop()
count = #lines
env.LuaExportAfterNextFrame()
assert(#lines == count, "Capture must stop")
assert(chains == 15 and #errors == 0, "Existing callbacks must be chained")
local text = table.concat(lines, "\n")
assert(text:find('\\"n\\":3', 1, true), "Multiple return counts must be retained")
assert(text:find('\\"present\\":false', 1, true), "Nil return position must be retained")
assert(text:find("restricted", 1, true), "API errors must be visible")
assert(text:find("Cyclic", 1, true), "Cyclic table failures must be visible")
assert(text:find("\\u00c3\\u00a9", 1, true), "String bytes must survive transport")
assert(text:find('"value":252000000', 1, true), "Raw frequency changes must be preserved in Hz")
assert(text:find('"85":0.75', 1, true) and text:find('"85":1', 1, true),
    "Launch bar candidate must preserve raw external state changes")
local externalAPI, getDevice = env.LoGetAircraftDrawArgumentValue, env.GetDevice
env.LoGetAircraftDrawArgumentValue = nil
env.GetDevice = function(id)
    if id == 0 then return getDevice(id) end
    return nil
end
env.LuaExportStart()
env.LuaExportAfterNextFrame()
local missing = table.concat(lines, "\n")
assert(missing:find("Radio device unavailable", 1, true), "Missing radios must be reported")
assert(missing:find("LoGetAircraftDrawArgumentValue", 1, true), "Missing external API must be reported")
env.LoGetAircraftDrawArgumentValue, env.GetDevice = externalAPI, getDevice
time = 8
env.LuaExportAfterNextFrame()
env.LuaExportStop()
assert(#errors == 0, "Unavailable optional streams must not disable cockpit recording")
text = table.concat(lines, "\n")
if arg[1] then
    local file = assert(io.open(arg[1], "wb"))
    assert(file:write(text .. "\n"))
    assert(file:close())
end
print("Broad native capture Lua tests passed")
