/**
 * @file Hornet.h
 * @brief Hornet Link v2 - F/A-18C panel library. Include this in your sketch.
 *
 * - Generated control names:   Hornet::MasterArm::MasterArm, Hornet::UFC::Key1, ...
 *                               (docs/F18C_CONTROL_REFERENCE.md lists them all)
 * - Elements:                  Switch, Selector, Button, Pot, Encoder, Lamp, Gauge, TextDisplay
 * - Wiring:                    pin(), analogPin(), output(), Hc165, Hc595, Matrix, Cd4067
 *                               (MCP23017: also include <HornetMcp23017.h>)
 * - Boards:                    Panel (USB, RS-485 slave or debug text), BusMaster
 *
 * Start with docs/FIRST_PANEL.md. The protocol is described in docs/PROTOCOL_V2.md.
 * The v1 (DCS-BIOS address based) API stays available through <HornetLink.h>.
 */

#pragma once

#include "HornetPanel.h"
