-- luacheck configuration for the DCS World export scripts.
std = "lua51"

-- Export hooks the script chains onto (read previous value, then redefine).
globals = {
    "LuaExportStart",
    "LuaExportStop",
    "LuaExportActivityNextEvent",
    "LuaExportAfterNextFrame",
    "LuaExportBeforeNextFrame",
    "ExportReceiveData",
}

-- Provided by the DCS export environment.
read_globals = {
    "socket",
    "lfs",
    "log",
    "LoGetSelf",
    "LoGetSelfData",
    "LoGetModelTime",
    "GetDevice",
    "list_indication",
}

max_line_length = false
