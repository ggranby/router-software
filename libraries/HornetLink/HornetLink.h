/**
 * @file HornetLink.h
 * @brief HornetLink Arduino library — top-level include.
 *
 * Include this single header to pull in all HornetLink
 * functionality:
 *
 *  - HornetLinkBase   — shared constants, flags, frame builders
 *  - HornetLinkMode   — operating-mode frame handling (Sim / Preflight / Maintenance)
 *  - HornetLinkMaster — RS-485 bus master (Mega 2560, ESP32)
 *  - HornetLinkSlave  — RS-485 bus slave  (Pro Micro, Nano)
 *  - HornetLinkImport — outbound import-command sender
 *  - HornetLinkCompatDcsBios — DCS-BIOS library compatibility shim
 *
 * @copyright Apache-2.0
 */

#pragma once

#include "src/HornetLinkBase.h"
#include "src/HornetLinkMode.h"
#include "src/HornetLinkMaster.h"
#include "src/HornetLinkSlave.h"
#include "src/HornetLinkImport.h"
#include "src/HornetLinkCompatDcsBios.h"
