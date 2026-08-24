#pragma once

#include "../include/color.h"

#include <cstdint>
#include <string>

// CSI (Control Sequence Introducer) sequences: everything this program writes as
// "ESC [ ...". Covers the SGR color and attribute sequences plus the screen and
// cursor control this renderer needs.
namespace csi {

// Whether a color applies to the glyph or to the cell behind it.
enum class Layer {
    Foreground,
    Background
};

// ANSI colors 0-15, in the order the SGR codes address them. These are the default
// VGA values; a terminal theme may paint them differently. The fourth component is
// the SGR background code; the matching foreground code is always 10 less.
struct PaletteEntry {
    std::uint8_t red;
    std::uint8_t green;
    std::uint8_t blue;
    int background;
};

constexpr int paletteSize = 16;
extern const PaletteEntry palette[paletteSize];

// Asks the terminal to resize its window to the given character grid (XTWINOPS).
// Terminals that disallow window manipulation ignore it.
std::string resize(int columns, int rows);

std::string eraseDisplay();
std::string cursorHome();
std::string cursorTo(int row, int column);
// Device Status Report, cursor position. The terminal answers ESC[row;colR, but only
// after it has parsed everything queued ahead of the query. See fence.h.
std::string deviceStatusReport();
// Primary Device Attributes. Like DSR it is answered only after everything queued
// ahead of it has been parsed, so it works as a fence too; some terminals treat the
// two queries differently, which is why both are offered.
std::string primaryDeviceAttributes();
// DEC private mode 2026, synchronized output. Between these the terminal buffers
// updates and presents them as one frame, which removes tearing. It defers the
// present; it does not report when the present happened.
std::string beginSynchronizedUpdate();
std::string endSynchronizedUpdate();
std::string hideCursor();
std::string showCursor();

std::string reset();
std::string bold(bool enabled);
std::string color16(int paletteIndex, Layer layer);
std::string color256(Color256 color, Layer layer);
std::string colorRgb(const Rgb& color, Layer layer);

}  // namespace csi
