/**
 * @file HornetCatalog.h
 * @brief Name lookups over the generated F/A-18C catalogue table.
 *
 * Used by the ASCII debug mode and the wiring test on the boards, and by the
 * bridge and tests on the PC. Works with the table in AVR flash.
 */

#pragma once

#include "generated/HornetF18C.h"

namespace Hornet {

/// Copy one catalogue row into RAM. Returns false if @p id is not in the catalogue.
inline bool findControl(uint16_t id, ControlInfo& out) {
    const ControlInfo* table = catalogTable();
    uint16_t lo = 0, hi = kCatalogControlCount;
    while (lo < hi) {
        const uint16_t mid = static_cast<uint16_t>((lo + hi) / 2);
        ControlInfo row;
        hn_memcpy_P(&row, &table[mid], sizeof(row));
        if (row.id == id) { out = row; return true; }
        if (row.id < id) lo = static_cast<uint16_t>(mid + 1); else hi = mid;
    }
    return false;
}

/// Copy a flash string (at most cap-1 chars). Returns the length copied.
inline uint8_t copyFlashString(const char* src, char* dst, uint8_t cap) {
    uint8_t n = 0;
    if (!src || cap == 0) { if (cap) dst[0] = '\0'; return 0; }
    while (n + 1 < cap) {
        const char c = hn_read_char_P(src + n);
        if (!c) break;
        dst[n++] = c;
    }
    dst[n] = '\0';
    return n;
}

/// Write "PANEL.CONTROL" for @p id, or "0xPPCC" if unknown.
inline uint8_t controlName(uint16_t id, char* dst, uint8_t cap) {
    ControlInfo row;
    if (findControl(id, row)) return copyFlashString(row.name, dst, cap);
    static const char hex[] = "0123456789ABCDEF";
    if (cap < 7) { if (cap) dst[0] = '\0'; return 0; }
    dst[0] = '0'; dst[1] = 'x';
    for (uint8_t i = 0; i < 4; i++) dst[2 + i] = hex[(id >> (12 - 4 * i)) & 0xF];
    dst[6] = '\0';
    return 6;
}

/// Write the name of position @p pos ("ARM"), or its number if the control has no names.
inline uint8_t positionName(const ControlInfo& row, uint16_t pos, char* dst, uint8_t cap) {
    if (cap == 0) return 0;
    if (row.positions && pos < row.count) {
        uint16_t field = 0;
        uint8_t n = 0;
        for (const char* p = row.positions;; p++) {
            const char c = hn_read_char_P(p);
            if (c == '\0') break;
            if (c == '|') { if (field++ == pos) break; continue; }
            if (field == pos && n + 1 < cap) dst[n++] = c;
        }
        dst[n] = '\0';
        return n;
    }
    // Plain number.
    char tmp[6];
    uint8_t t = 0;
    do { tmp[t++] = static_cast<char>('0' + pos % 10); pos = static_cast<uint16_t>(pos / 10); } while (pos && t < 5);
    uint8_t n = 0;
    while (t && n + 1 < cap) dst[n++] = tmp[--t];
    dst[n] = '\0';
    return n;
}

/// Case-sensitive lookup of "PANEL.CONTROL". Linear scan; meant for debug input.
inline bool findControlByName(const char* name, ControlInfo& out) {
    const ControlInfo* table = catalogTable();
    for (uint16_t i = 0; i < kCatalogControlCount; i++) {
        ControlInfo row;
        hn_memcpy_P(&row, &table[i], sizeof(row));
        uint16_t k = 0;
        for (;; k++) {
            const char c = hn_read_char_P(row.name + k);
            if (c != name[k]) break;
            if (c == '\0') { out = row; return true; }
        }
    }
    return false;
}

/// Position index for a position name ("ARM" -> 1). Returns -1 if not found.
inline int positionIndex(const ControlInfo& row, const char* name) {
    if (!row.positions) return -1;
    int field = 0;
    uint8_t k = 0;
    bool match = true;
    for (const char* p = row.positions;; p++) {
        const char c = hn_read_char_P(p);
        if (c == '|' || c == '\0') {
            if (match && name[k] == '\0') return field;
            if (c == '\0') return -1;
            field++;
            k = 0;
            match = true;
            continue;
        }
        if (match && name[k] == c) k++; else match = false;
    }
}

} // namespace Hornet
