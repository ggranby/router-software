-- Run from the repository root with Lua 5.1.
local script = "Programs/dcsbios-serial-bridge/lua/HornetLinkVerify.lua"
local expected = {44, 45, 46, 47, 48, 49, 50, 458, 459}

local function fixture(options)
    options = options or {}
    local state = {
        rows = "", errors = {}, time = 0, aircraft = "FA-18C_hornet",
        values = {}, reads = 0, chained = 0,
    }
    for _, arg in ipairs(expected) do state.values[arg] = 0 end
    local function chain() state.chained = state.chained + 1 end
    local env = setmetatable({
        io = {},
        log = {INFO = 1, ERROR = 2, write = function(_, level, message)
            if level == 2 then state.errors[#state.errors + 1] = message end
            if message:sub(1, 4) == "CSV " then
                state.rows = state.rows .. message:sub(5) .. "\n"
            end
        end},
        LoGetSelfData = function()
            if state.aircraft then return {Name = state.aircraft} end
        end,
        LoGetModelTime = function() return state.time end,
        GetDevice = function(id)
            assert(id == 0, "Verifier must not access an input device")
            if options.noDevice then return nil end
            return {get_argument_value = function(_, arg)
                assert(state.values[arg] ~= nil, "Unexpected argument read")
                state.reads = state.reads + 1
                return state.values[arg]
            end}
        end,
        LuaExportStart = chain,
        LuaExportAfterNextFrame = chain,
        LuaExportStop = chain,
    }, {__index = _G})
    local chunk = assert(loadfile(script))
    setfenv(chunk, env)()
    state.hooks = env
    env.LuaExportStart()
    return state
end

local state = fixture()
state.hooks.LuaExportAfterNextFrame()
assert(state.reads == 9)
for _, arg in ipairs(expected) do
    assert(state.rows:find(",full," .. arg .. ",0\n", 1, true))
end
local before = state.rows
state.time = 0.1
state.hooks.LuaExportAfterNextFrame()
assert(state.rows == before, "Unchanged values should not produce delta records")
state.values[49] = 0.125
state.time = 0.2
state.hooks.LuaExportAfterNextFrame()
assert(state.rows:find(",0.200000,change,49,0.125\n", 1, true),
    "Raw argument values must not be thresholded or normalized")
state.time = 1
state.hooks.LuaExportAfterNextFrame()
assert(state.rows:find(",1.000000,full,49,0.125\n", 1, true))
state.time = 0
state.hooks.LuaExportAfterNextFrame()
assert(state.rows:find(",0.000000,full,49,0.125\n", 1, true), "Clock reset needs a full snapshot")
local reads = state.reads
state.aircraft = "Other"
state.hooks.LuaExportAfterNextFrame()
assert(state.reads == reads and state.rows:find(",inactive,,\n", 1, true))
state.aircraft = false
state.hooks.LuaExportAfterNextFrame()
assert(state.reads == reads)
state.aircraft = "FA-18C_hornet"
state.hooks.LuaExportAfterNextFrame()
state.hooks.LuaExportStop()
assert(state.chained == 10 and #state.errors == 0)
assert(state.rows:find(",stop,,\n", 1, true))
local stopped = state.rows
state.hooks.LuaExportAfterNextFrame()
assert(state.rows == stopped, "No samples may be captured after export stop")
state.hooks.LuaExportStart()
state.hooks.LuaExportStop()
local _, starts = state.rows:gsub(",start,,", "")
assert(starts == 2, "Restart must emit a new session boundary")

local broken = fixture({noDevice = true})
broken.hooks.LuaExportAfterNextFrame()
broken.hooks.LuaExportStop()
assert(#broken.errors == 1, "Missing cockpit device must be reported")
broken.hooks.LuaExportAfterNextFrame()
assert(#broken.errors == 1, "Capture failure should disable capture without log flooding")
local invalid = fixture()
invalid.values[44] = 0 / 0
invalid.hooks.LuaExportAfterNextFrame()
assert(#invalid.errors == 1)
local readsBefore = invalid.reads
invalid.hooks.LuaExportAfterNextFrame()
assert(invalid.reads == readsBefore and #invalid.errors == 1)
invalid.values[44] = 0
invalid.hooks.LuaExportStart()
invalid.hooks.LuaExportAfterNextFrame()
assert(invalid.reads > readsBefore, "Next export start must recover from a previous capture failure")
local badTime = fixture()
badTime.time = math.huge
badTime.hooks.LuaExportAfterNextFrame()
assert(#badTime.errors == 1 and badTime.reads == 0)
print("Native verification capture tests passed")
