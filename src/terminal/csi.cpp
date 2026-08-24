#include "csi.h"

#include <cstdio>

namespace csi {
namespace {

// Wide enough for any int, including the "-2147483648" edge. snprintf reports the
// length it wanted rather than what it wrote, so a short buffer would make the
// append below read past the end of it.
void appendNumber(std::string& output, int value) {
    char buffer[12]{};
    const int length = std::snprintf(buffer, sizeof(buffer), "%d", value);
    if (length > 0) {
        output.append(buffer, static_cast<std::size_t>(length));
    }
}

}  // namespace

const PaletteEntry palette[paletteSize] = {
    {0, 0, 0,          40},   // 0  black
    {128, 0, 0,        41},   // 1  red
    {0, 128, 0,        42},   // 2  green
    {128, 128, 0,      43},   // 3  yellow
    {0, 0, 128,        44},   // 4  blue
    {128, 0, 128,      45},   // 5  magenta
    {0, 128, 128,      46},   // 6  cyan
    {192, 192, 192,    47},   // 7  white
    {128, 128, 128,   100},   // 8  bright black
    {255, 0, 0,       101},   // 9  bright red
    {0, 255, 0,       102},   // 10 bright green
    {255, 255, 0,     103},   // 11 bright yellow
    {0, 0, 255,       104},   // 12 bright blue
    {255, 0, 255,     105},   // 13 bright magenta
    {0, 255, 255,     106},   // 14 bright cyan
    {255, 255, 255,   107}    // 15 bright white
};

std::string resize(int columns, int rows) {
    std::string sequence = "\x1b[8;";
    appendNumber(sequence, rows);
    sequence += ';';
    appendNumber(sequence, columns);
    sequence += 't';
    return sequence;
}

std::string eraseDisplay() {
    return "\x1b[2J";
}

std::string cursorHome() {
    return "\x1b[H";
}

std::string cursorTo(int row, int column) {
    std::string sequence = "\x1b[";
    appendNumber(sequence, row);
    sequence += ';';
    appendNumber(sequence, column);
    sequence += 'H';
    return sequence;
}

std::string deviceStatusReport() {
    return "\x1b[6n";
}

std::string hideCursor() {
    return "\x1b[?25l";
}

std::string showCursor() {
    return "\x1b[?25h";
}

std::string reset() {
    return "\x1b[0m";
}

std::string bold(bool enabled) {
    return enabled ? "\x1b[1m" : "\x1b[22m";
}

std::string color16(int paletteIndex, Layer layer) {
    std::string sequence = "\x1b[";
    appendNumber(sequence, palette[paletteIndex].background - (layer == Layer::Foreground ? 10 : 0));
    sequence += 'm';
    return sequence;
}

std::string color256(Color256 color, Layer layer) {
    std::string sequence = layer == Layer::Foreground ? "\x1b[38;5;" : "\x1b[48;5;";
    appendNumber(sequence, color.value);
    sequence += 'm';
    return sequence;
}

std::string colorRgb(const Rgb& color, Layer layer) {
    std::string sequence = layer == Layer::Foreground ? "\x1b[38;2;" : "\x1b[48;2;";
    appendNumber(sequence, color.red);
    sequence += ';';
    appendNumber(sequence, color.green);
    sequence += ';';
    appendNumber(sequence, color.blue);
    sequence += 'm';
    return sequence;
}

}  // namespace csi
