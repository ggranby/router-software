--[[
  HornetLinkExport.lua
  DCS-BIOS Hornet Link Exporter
  ============================================================
  Drop this file (or require it from Export.lua) inside your
  DCS Saved Games Scripts folder.  It opens a UDP socket and
  forwards every DCS-BIOS export update to Hornet Link
  (hornet-link.exe) listening on 127.0.0.1:42002.

  The packet format is raw DCS-BIOS: a 4-byte sync header
  followed by address/length/data write records, exactly as
  produced by the DCS-BIOS stream—so the bridge needs no
  extra parsing beyond what it already does for the UDP
  multicast stream.

  INSTALLATION
  ──────────────────────────────────────────────────────────
  In %USERPROFILE%\Saved Games\DCS[.openbeta]\Scripts\Export.lua
  add at the very end:

      local hlOk, hlErr = pcall(dofile,
          lfs.writedir() .. "Scripts/HornetLinkExport.lua")
      if not hlOk then log.write("HornetLink", log.ERROR, tostring(hlErr)) end

  PROTOCOL
  ──────────────────────────────────────────────────────────
  Each UDP datagram contains exactly one DCS-BIOS frame:
      [0x55 0x55 0x55 0x55]           sync word
      [addr_lo addr_hi len_lo len_hi] write-record header
      [data ... len bytes]            payload

  Multiple write records may follow a single sync word
  (batched when many addresses change in the same frame).

  The bridge (DcsDirectSource) re-assembles them using the
  same ExportParser used for the multicast stream.
]]

-- ── Configuration ────────────────────────────────────────
local HL_HOST = "127.0.0.1"
local HL_PORT = 42002

-- ── Socket ──────────────────────────────────────────────
local hl_socket  = nil
local hl_addr    = nil
local hl_buf     = ""      -- accumulates bytes for this frame
local hl_synced  = false   -- true after first sync is appended

-- ── Helpers ─────────────────────────────────────────────
local function hl_open()
    hl_socket = socket.udp()
    hl_socket:settimeout(0)
    hl_addr = { ip = HL_HOST, port = HL_PORT }
end

local function hl_u16le(v)
    v = math.floor(v) % 65536
    return string.char(v % 256, math.floor(v / 256) % 256)
end

-- ── DCS Export hooks ────────────────────────────────────
local hl_prev_LuaExportStart = LuaExportStart
function LuaExportStart()
    hl_open()
    if hl_prev_LuaExportStart then hl_prev_LuaExportStart() end
end

local hl_prev_LuaExportStop = LuaExportStop
function LuaExportStop()
    if hl_socket then hl_socket:close(); hl_socket = nil end
    if hl_prev_LuaExportStop then hl_prev_LuaExportStop() end
end

-- Called every export cycle (≈ 30 Hz by default).
-- Build one UDP datagram per cycle: sync + all dirty records.
local hl_prev_LuaExportActivityNextEvent = LuaExportActivityNextEvent
function LuaExportActivityNextEvent(t)
    local nextTime = t
    if hl_prev_LuaExportActivityNextEvent then
        nextTime = hl_prev_LuaExportActivityNextEvent(t)
    end

    if not hl_socket then return nextTime end

    -- Collect dirty words from DCS LoGetWorld / LoGetSelf if available.
    -- The primary mechanism is the BeforeNextFrame hook below which
    -- intercepts the DCS-BIOS data stream.
    return nextTime
end

-- DCS-BIOS hooks into this global to receive raw export bytes.
-- We shadow it so we can capture the bytes and forward them.
local hl_prev_ExportReceiveData = ExportReceiveData
function ExportReceiveData(data)
    if hl_prev_ExportReceiveData then hl_prev_ExportReceiveData(data) end

    if not hl_socket or not data or #data == 0 then return end
    -- 'data' is already a DCS-BIOS stream chunk.  Wrap it in a sync
    -- header if it doesn't already start with 0x55 0x55 0x55 0x55.
    -- In practice DCS-BIOS prepends the sync for each frame; we just
    -- forward verbatim.
    local ok, err = hl_socket:sendto(data, HL_HOST, HL_PORT)
    if not ok and err then
        -- Silently ignore EAGAIN (socket buffer full); re-open on hard error.
        if err ~= "timeout" and err ~= "Resource temporarily unavailable" then
            hl_socket:close()
            hl_open()
        end
    end
end

-- Fallback: if DCS-BIOS is NOT installed, synthesise a minimal export
-- from LoGetSelf (attitude, altitude, speed) so testing is possible.
-- Only active when ExportReceiveData was not already hooked by DCS-BIOS.
local hl_dcsbios_present = (hl_prev_ExportReceiveData ~= nil)
if not hl_dcsbios_present then
    local hl_prev_after = LuaExportAfterNextFrame
    function LuaExportAfterNextFrame()
        if hl_prev_after then hl_prev_after() end
        if not hl_socket then return end

        -- Minimal synthetic frame: aircraft altitude at a known address.
        -- Address 0x0000 is used as a placeholder "alive" heartbeat word.
        local alt = 0
        local self_data = LoGetSelf()
        if self_data and self_data.position then
            alt = math.floor(self_data.position.y)
        end

        local frame =
            "\x55\x55\x55\x55" ..   -- sync
            hl_u16le(0x0000)     ..  -- address 0x0000
            hl_u16le(2)          ..  -- length = 2 bytes
            hl_u16le(alt % 65536)    -- altitude word

        hl_socket:sendto(frame, HL_HOST, HL_PORT)
    end
end
