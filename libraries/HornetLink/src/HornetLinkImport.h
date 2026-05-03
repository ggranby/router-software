/**
 * @file HornetLinkImport.h
 * @brief Outbound import-command sender for bidirectional devices.
 *
 * Provides a simple API to queue and send DCS-BIOS import command
 * strings (e.g. "SET UFC_KEY_1 1\n") back to the PC via the
 * primary serial port.
 */

#pragma once
#include "HornetLinkBase.h"

class HornetLinkImport {
public:
    /**
     * @brief Construct an import sender.
     * @param stream  The serial stream connected to the PC (or master).
     */
    explicit HornetLinkImport(Stream& stream) : stream_(stream) {}

    /**
     * @brief Send a DCS-BIOS import command.
     *
     * Formats the string as:  SET <name> <value>\n
     *
     * @param control  DCS-BIOS control name (e.g. "UFC_KEY_1").
     * @param value    Integer value to set (e.g. 1).
     */
    void sendCommand(const char* control, uint16_t value) {
        stream_.print("SET ");
        stream_.print(control);
        stream_.print(' ');
        stream_.print(value);
        stream_.print('\n');
    }

    /**
     * @brief Send a raw pre-formatted import line.
     * @param line  Null-terminated import string.  A newline is appended
     *              if @p line does not already end with one.
     */
    void sendRaw(const char* line) {
        stream_.print(line);
        size_t n = strlen(line);
        if (n == 0 || line[n - 1] != '\n') stream_.print('\n');
    }

private:
    Stream& stream_;
};
