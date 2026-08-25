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
// Primary Device Attributes. The terminal answers ESC[?...c, but only after it has
// parsed everything queued ahead of the query. See fence.h.
std::string primaryDeviceAttributes();
std::string hideCursor();
std::string showCursor();

std::string reset();
std::string bold(bool enabled);
std::string color16(int paletteIndex, Layer layer);
std::string color256(Color256 color, Layer layer);
std::string colorRgb(const Rgb& color, Layer layer);

}  // namespace csi
