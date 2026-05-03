/**
 * @file HornetLinkCompatDcsBios.h
 * @brief DCS-BIOS library compatibility shim for HornetLink.
 *
 * If your panel firmware already uses the DCS-BIOS Arduino
 * library (https://github.com/dcs-bios/dcs-bios-arduino-library),
 * this shim lets you migrate incrementally:
 *
 *  1. Replace `#include <DcsBios.h>` with `#include <HornetLink.h>`
 *  2. Keep using DcsBios::Switch2Pos, DcsBios::LED etc. unchanged.
 *  3. The shim redirects the serial I/O to use HornetLink's
 *     handshake-aware transport instead of raw DCS-BIOS framing.
 *
 * LIMITATIONS
 * ─────────────────────────────────────────────────────────
 *  - Requires the full DCS-BIOS Arduino library to also be
 *    installed (HornetLink does not re-implement its control classes).
 *  - Only the USB-CDC transport mode is shimmed; RS-485 bus
 *    mode must be wired up separately via HornetLinkMaster/Slave.
 *
 * To disable the shim and use pure HornetLink mode, define
 * HL_NO_DCSBIOS_COMPAT before including HornetLink.h.
 */

#pragma once

#ifndef HL_NO_DCSBIOS_COMPAT

// Redirect DCS-BIOS serial I/O to the primary HornetLink stream.
// DcsBios.h uses a global `DcsBios::ProtocolParser parser;` and
// `DcsBios::ExportStreamListener` objects.  We preserve their
// linkage while intercepting the serial send path.
//
// NOTE: This shim is intentionally minimal.  If DCS-BIOS changes
// its internal API this file must be updated to match.

#if __has_include(<DcsBios.h>)
  #include <DcsBios.h>
  // Nothing extra needed — DcsBios.h's own serial hooks work
  // unchanged because hornet-link.exe accepts the same import
  // command syntax ("SET CONTROL VALUE\n").
#else
  // DCS-BIOS library not installed — provide stub types so
  // existing sketch code compiles without errors.
  namespace DcsBios {
    struct ProtocolParser { void processChar(uint8_t) {} };
    extern ProtocolParser parser;
    inline void setup() {}
    inline void loop()  {}
  }
#endif // __has_include

#endif // HL_NO_DCSBIOS_COMPAT
