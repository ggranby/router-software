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

#include "HornetLinkBase.h"
#include "HornetLinkMode.h"
#include "HornetLinkMaster.h"
#include "HornetLinkSlave.h"
#include "HornetLinkImport.h"
#include "HornetLinkCompatDcsBios.h"
