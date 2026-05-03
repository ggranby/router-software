--[[
  FA-18C.lua — F/A-18C Hornet DCS-BIOS Address Map
  ============================================================
  Reference table of DCS-BIOS addresses and masks for the
  F/A-18C Hornet module.  Used by HornetLinkExport.lua to
  optionally filter which addresses are forwarded, and by
  firmware authors to look up field encodings.

  Format:
    address  — even byte offset into the 65536-byte state map
    mask     — bitmask isolating the field within the 16-bit word
    shift    — right-shift to extract the raw integer value
    maxval   — maximum raw value (0xFFFF for full-word analogue)
    name     — DCS-BIOS identifier string

  Sources:
    - DCS-BIOS open-source address tables (Apache-2.0)
    - OpenHornet panel schematics

  NOTE: Addresses are decimal for easy math; use bit32.bor /
  bit32.band in Lua.  Firmware typically receives these as
  little-endian 16-bit words.
]]

local FA18C = {}

-- ── UFC ──────────────────────────────────────────────────
FA18C.UFC_COMM1_DISPLAY          = { address = 0x740A, mask = 0x00FF, shift = 0,  maxval = 0x00FF }
FA18C.UFC_COMM2_DISPLAY          = { address = 0x740A, mask = 0xFF00, shift = 8,  maxval = 0x00FF }
FA18C.UFC_SCRATCHPAD_NUMBER      = { address = 0x7406, mask = 0xFFFF, shift = 0,  maxval = 0xFFFF }
FA18C.UFC_SCRATCHPAD_STRING_1    = { address = 0x7408, mask = 0x00FF, shift = 0,  maxval = 0x00FF }
FA18C.UFC_SCRATCHPAD_STRING_2    = { address = 0x7408, mask = 0xFF00, shift = 8,  maxval = 0x00FF }
FA18C.UFC_OPTION_DISPLAY_1       = { address = 0x7400, mask = 0x000F, shift = 0,  maxval = 0x000F }
FA18C.UFC_OPTION_DISPLAY_2       = { address = 0x7400, mask = 0x00F0, shift = 4,  maxval = 0x000F }
FA18C.UFC_OPTION_DISPLAY_3       = { address = 0x7400, mask = 0x0F00, shift = 8,  maxval = 0x000F }
FA18C.UFC_OPTION_DISPLAY_4       = { address = 0x7400, mask = 0xF000, shift = 12, maxval = 0x000F }
FA18C.UFC_OPTION_DISPLAY_5       = { address = 0x7402, mask = 0x000F, shift = 0,  maxval = 0x000F }

-- ── Master Arm ───────────────────────────────────────────
FA18C.MASTER_ARM_SW              = { address = 0x740C, mask = 0x0003, shift = 0,  maxval = 3 }
FA18C.MC_ARM_SW                  = { address = 0x740C, mask = 0x000C, shift = 2,  maxval = 3 }
FA18C.MASTER_MODE_AA             = { address = 0x740C, mask = 0x0010, shift = 4,  maxval = 1 }
FA18C.MASTER_MODE_AG             = { address = 0x740C, mask = 0x0020, shift = 5,  maxval = 1 }
FA18C.FIRE_BTN_COVER             = { address = 0x740C, mask = 0x00C0, shift = 6,  maxval = 3 }

-- ── Engines ──────────────────────────────────────────────
FA18C.ENG_L_RPM                  = { address = 0x0446, mask = 0xFFFF, shift = 0,  maxval = 0xFFFF }
FA18C.ENG_L_FTIT                 = { address = 0x0448, mask = 0xFFFF, shift = 0,  maxval = 0xFFFF }
FA18C.ENG_L_FF                   = { address = 0x044A, mask = 0xFFFF, shift = 0,  maxval = 0xFFFF }
FA18C.ENG_L_OIL                  = { address = 0x044C, mask = 0xFFFF, shift = 0,  maxval = 0xFFFF }
FA18C.ENG_L_NOZZLE               = { address = 0x044E, mask = 0xFFFF, shift = 0,  maxval = 0xFFFF }
FA18C.ENG_R_RPM                  = { address = 0x0452, mask = 0xFFFF, shift = 0,  maxval = 0xFFFF }
FA18C.ENG_R_FTIT                 = { address = 0x0454, mask = 0xFFFF, shift = 0,  maxval = 0xFFFF }
FA18C.ENG_R_FF                   = { address = 0x0456, mask = 0xFFFF, shift = 0,  maxval = 0xFFFF }
FA18C.ENG_R_OIL                  = { address = 0x0458, mask = 0xFFFF, shift = 0,  maxval = 0xFFFF }
FA18C.ENG_R_NOZZLE               = { address = 0x045A, mask = 0xFFFF, shift = 0,  maxval = 0xFFFF }

-- ── Fuel ─────────────────────────────────────────────────
FA18C.FUEL_L                     = { address = 0x04E8, mask = 0xFFFF, shift = 0,  maxval = 0xFFFF }
FA18C.FUEL_R                     = { address = 0x04EA, mask = 0xFFFF, shift = 0,  maxval = 0xFFFF }
FA18C.FUEL_TOTAL                 = { address = 0x04EC, mask = 0xFFFF, shift = 0,  maxval = 0xFFFF }
FA18C.FUEL_BINGO                 = { address = 0x04EE, mask = 0xFFFF, shift = 0,  maxval = 0xFFFF }

-- ── Altimeter / ADI ──────────────────────────────────────
FA18C.BARO_SET                   = { address = 0x0200, mask = 0xFFFF, shift = 0,  maxval = 0xFFFF }
FA18C.BARO_ALT                   = { address = 0x0202, mask = 0xFFFF, shift = 0,  maxval = 0xFFFF }
FA18C.ADI_PITCH                  = { address = 0x0204, mask = 0xFFFF, shift = 0,  maxval = 0xFFFF }
FA18C.ADI_ROLL                   = { address = 0x0206, mask = 0xFFFF, shift = 0,  maxval = 0xFFFF }

-- ── HSI ──────────────────────────────────────────────────
FA18C.HSI_HEADING                = { address = 0x0220, mask = 0xFFFF, shift = 0,  maxval = 0xFFFF }
FA18C.HSI_BEARING                = { address = 0x0222, mask = 0xFFFF, shift = 0,  maxval = 0xFFFF }
FA18C.HSI_CRS                    = { address = 0x0224, mask = 0xFFFF, shift = 0,  maxval = 0xFFFF }
FA18C.HSI_DISTANCE               = { address = 0x0226, mask = 0xFFFF, shift = 0,  maxval = 0xFFFF }
FA18C.HSI_DESIRED_HEADING        = { address = 0x0228, mask = 0xFFFF, shift = 0,  maxval = 0xFFFF }

-- ── Autopilot ────────────────────────────────────────────
FA18C.AP_ENGAGE_SW               = { address = 0x7480, mask = 0x0003, shift = 0,  maxval = 3 }
FA18C.AP_DISENGAGE_LIGHT         = { address = 0x7480, mask = 0x000C, shift = 2,  maxval = 1 }
FA18C.AP_ALT_SW                  = { address = 0x7480, mask = 0x0030, shift = 4,  maxval = 3 }
FA18C.AP_HDG_SW                  = { address = 0x7480, mask = 0x00C0, shift = 6,  maxval = 3 }

-- ── Caution Advisory Panel ───────────────────────────────
FA18C.CAUTION_LIGHTS_WORD0       = { address = 0x7400, mask = 0xFFFF, shift = 0,  maxval = 0xFFFF }
FA18C.CAUTION_LIGHTS_WORD1       = { address = 0x7402, mask = 0xFFFF, shift = 0,  maxval = 0xFFFF }
FA18C.CAUTION_LIGHTS_WORD2       = { address = 0x7404, mask = 0xFFFF, shift = 0,  maxval = 0xFFFF }

-- ── HUD ──────────────────────────────────────────────────
FA18C.HUD_SYM_REJ_SW             = { address = 0x7512, mask = 0x0003, shift = 0,  maxval = 3 }
FA18C.HUD_VELOCITY_SW            = { address = 0x7512, mask = 0x000C, shift = 2,  maxval = 3 }
FA18C.HUD_ALTITUDE_SW            = { address = 0x7512, mask = 0x0030, shift = 4,  maxval = 3 }
FA18C.HUD_BRIGHTNESS_SW          = { address = 0x7512, mask = 0x00C0, shift = 6,  maxval = 3 }
FA18C.HUD_BRIGHTNESS_KNOB        = { address = 0x7514, mask = 0xFFFF, shift = 0,  maxval = 0xFFFF }

-- ── Import commands (device → DCS) ───────────────────────
-- These are the string tokens used in ImportCommand lines
-- sent from firmware back to DCS via the serial port.
FA18C.import_commands = {
    UFC_COMM1_CHANNEL_SELECT    = "UFC_COMM1_CHANNEL_SELECT",
    UFC_COMM2_CHANNEL_SELECT    = "UFC_COMM2_CHANNEL_SELECT",
    UFC_KEY_0                   = "UFC_KEY_0",
    UFC_KEY_1                   = "UFC_KEY_1",
    UFC_KEY_2                   = "UFC_KEY_2",
    UFC_KEY_3                   = "UFC_KEY_3",
    UFC_KEY_4                   = "UFC_KEY_4",
    UFC_KEY_5                   = "UFC_KEY_5",
    UFC_KEY_6                   = "UFC_KEY_6",
    UFC_KEY_7                   = "UFC_KEY_7",
    UFC_KEY_8                   = "UFC_KEY_8",
    UFC_KEY_9                   = "UFC_KEY_9",
    UFC_KEY_CLR                 = "UFC_KEY_CLR",
    UFC_KEY_ENT                 = "UFC_KEY_ENT",
    MASTER_ARM_SW               = "MASTER_ARM_SW",
}

return FA18C
