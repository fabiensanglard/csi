#include "palette256_command.h"

#include <climits>
#include <cstdio>
#include <stdexcept>
#include <string>

namespace {

// The chart addresses palette slots directly rather than going through the
// terminal's RGB mapping: the point is to show what each index actually paints, so
// asking for a color by index is the whole exercise.

// Every cell is padded on both sides so neighbouring labels do not run together into
// one unreadable strip of digits. Index cells read " 0xXX ", cube cells " (r,g,b) ".
constexpr int indexCellWidth = 6;
constexpr int cubeCellWidth = 9;
// Sections stand three columns apart, except the two grey scales: those are kept a
// column closer so they read as a pair meant to be compared against each other.
constexpr int sectionGapWidth = 3;
constexpr int greyGapWidth = 2;

// The 16 system colors run down the left edge, one per row, split into the dark
// eight and the bright eight with a sublabel over each half and a blank line
// between. Those extra rows push the bright half down, which is why this column is
// addressed by row rather than indexed straight into.
constexpr int systemHalfRows = 8;
constexpr int systemDarkLabelRow = 0;
constexpr int systemDarkFirstRow = systemDarkLabelRow + 1;
constexpr int systemBrightLabelRow = systemDarkFirstRow + systemHalfRows + 1;
constexpr int systemBrightFirstRow = systemBrightLabelRow + 1;

// The cube is 6 blocks of green x blue, one block per red level, laid out two blocks
// across and three down, which keeps it 12 cells wide rather than 36.
constexpr int cubeBlocksAcross = 2;
constexpr int cubeBlocksDown = 3;
constexpr int cubeBlockSize = 6;
constexpr int cubeColumns = cubeBlocksAcross * cubeBlockSize;
constexpr int cubeRows = cubeBlocksDown * cubeBlockSize;

constexpr int rampRows = 24;
constexpr int rampFirstIndex = 232;

// The cube's own greys: the six cells where red, green and blue all match. The
// flattened layout puts that diagonal at a different spot inside every block, so the
// six never line up; gathering them here is what makes them visible as a scale.
constexpr int cubeGreyRows = 6;
constexpr int cubeGreyStride = 43;  // 36 + 6 + 1, one step along the r=g=b diagonal

constexpr int swatchRows = rampRows;  // the tallest of the four columns
constexpr int headerRows = 2;  // the headings, then a blank line under them
constexpr int chartRows = headerRows + swatchRows;
constexpr int cubeWidth = cubeColumns * cubeCellWidth;
constexpr int chartColumns = indexCellWidth + sectionGapWidth + cubeWidth + sectionGapWidth
                           + indexCellWidth + greyGapWidth + cubeCellWidth;

// The cube's own black and white, used for the labels. Indices 0 and 15 would be the
// obvious choice but they are themed, so the text could come out invisible; these
// two have spec-defined values.
constexpr int labelDark = 16;
constexpr int labelLight = 231;

constexpr int levels[] = {0, 95, 135, 175, 215, 255};

// What a palette slot is meant to paint. The system colors are the VGA defaults, so
// for those this is the usual guess: a theme may paint them differently, and the
// label contrast is only as good as that guess.
Rgb paletteColor(int index) {
    if (index < csi::paletteSize) {
        const csi::PaletteEntry& entry = csi::palette[index];
        return {entry.red, entry.green, entry.blue};
    }
    if (index < rampFirstIndex) {
        const int offset = index - csi::paletteSize;
        return {static_cast<std::uint8_t>(levels[offset / 36]),
                static_cast<std::uint8_t>(levels[(offset / 6) % 6]),
                static_cast<std::uint8_t>(levels[offset % 6])};
    }
    const auto level = static_cast<std::uint8_t>(8 + (index - rampFirstIndex) * 10);
    return {level, level, level};
}

// Rec. 601 luma, which tracks perceived brightness closely enough to decide whether
// dark or light text reads better on top of a swatch.
bool prefersDarkLabel(const Rgb& color) {
    const int luma = (299 * color.red + 587 * color.green + 114 * color.blue) / 1000;
    return luma > 128;
}

int cubeGreyIndex(int step) {
    return csi::paletteSize + cubeGreyStride * step;
}

int rampLevel(int row) {
    return 8 + row * 10;
}

// Which ramp step a given brightness sits closest to.
int nearestRampRow(int level) {
    int bestRow = 0;
    int bestDistance = INT_MAX;
    for (int row = 0; row < rampRows; ++row) {
        const int difference = level - rampLevel(row);
        const int distance = difference < 0 ? -difference : difference;
        if (distance < bestDistance) {
            bestRow = row;
            bestDistance = distance;
        }
    }
    return bestRow;
}

// The cube grey belonging on this row, or -1 for a row that has none. Each one is
// parked beside the ramp step nearest it in brightness rather than stacked at the
// top, which is what lets the two scales be read against each other: the ramp climbs
// in tens while the cube jumps 0, 95, 135, 175, 215, 255. Those six levels land on
// six distinct ramp rows, so none of them is crowded out.
int cubeGreyForRow(int row) {
    for (int step = 0; step < cubeGreyRows; ++step) {
        if (nearestRampRow(levels[step]) == row) {
            return step;
        }
    }
    return -1;
}

// Each 6x6 block is one slice of constant red, green down and blue across, so
// neighbouring cells differ in a single coordinate and the cube's structure stays
// visible.
int cubeIndex(int row, int column) {
    const int red = (row / cubeBlockSize) * cubeBlocksAcross + column / cubeBlockSize;
    const int green = row % cubeBlockSize;
    const int blue = column % cubeBlockSize;
    return csi::paletteSize + 36 * red + 6 * green + blue;
}

// Paints one swatch in the color of the given index and writes text over it. The
// text is expected to already fill the cell.
void writeSwatch(Terminal& terminal, int index, const std::string& text) {
    const Rgb color = paletteColor(index);
    const int label = prefersDarkLabel(color) ? labelDark : labelLight;
    terminal.writeText(csi::color256({static_cast<std::uint8_t>(index)}, csi::Layer::Background));
    terminal.writeText(csi::color256({static_cast<std::uint8_t>(label)}, csi::Layer::Foreground));
    terminal.writeText(text);
}

std::string indexText(int index) {
    char text[indexCellWidth + 1]{};
    std::snprintf(text, sizeof(text), " 0x%02X ", index);
    return text;
}

// Cube cells carry their coordinates instead of their index, so the structure of the
// 6x6x6 grid is readable straight off the chart.
std::string cubeText(int index) {
    const int offset = index - csi::paletteSize;
    char text[cubeCellWidth + 1]{};
    std::snprintf(text, sizeof(text), " (%d,%d,%d) ", offset / 36, (offset / 6) % 6, offset % 6);
    return text;
}

void writeBlank(Terminal& terminal, int columns) {
    terminal.resetColor();
    terminal.writeText(std::string(static_cast<std::size_t>(columns), ' '));
}

void writeSectionGap(Terminal& terminal) {
    writeBlank(terminal, sectionGapWidth);
}

void writeGreyGap(Terminal& terminal) {
    writeBlank(terminal, greyGapWidth);
}

// Centered over the column it names, so a heading sits above the middle of its
// section rather than hugging the left edge of a wide one.
std::string centered(const char* text, int width) {
    std::string label = text;
    if (static_cast<int>(label.size()) > width) {
        label.resize(static_cast<std::size_t>(width));
    }
    const auto leading = static_cast<std::size_t>((width - static_cast<int>(label.size())) / 2);
    std::string padded(leading, ' ');
    padded += label;
    padded.resize(static_cast<std::size_t>(width), ' ');
    return padded;
}

// Section headings sit on the default background, above their column. The system and
// grey columns are only one cell wide, so their headings are kept short enough to
// fit inside that cell.
void writeHeading(Terminal& terminal, const char* text, int width) {
    terminal.resetColor();
    terminal.setBold(true);
    terminal.writeText(centered(text, width));
    terminal.setBold(false);
}

// Sublabels are plain rather than bold, so they read as a division inside a section
// instead of competing with the section headings.
void writeSubheading(Terminal& terminal, const char* text, int width) {
    terminal.resetColor();
    terminal.writeText(centered(text, width));
}

void writeSystemCell(Terminal& terminal, int row) {
    if (row == systemDarkLabelRow) {
        writeSubheading(terminal, "Dark", indexCellWidth);
        return;
    }
    if (row == systemBrightLabelRow) {
        writeSubheading(terminal, "Bright", indexCellWidth);
        return;
    }
    if (row >= systemDarkFirstRow && row < systemDarkFirstRow + systemHalfRows) {
        const int index = row - systemDarkFirstRow;
        writeSwatch(terminal, index, indexText(index));
        return;
    }
    if (row >= systemBrightFirstRow && row < systemBrightFirstRow + systemHalfRows) {
        const int index = systemHalfRows + row - systemBrightFirstRow;
        writeSwatch(terminal, index, indexText(index));
        return;
    }
    writeBlank(terminal, indexCellWidth);
}

}  // namespace

void Palette256Command::parseArgs(int argc, char* argv[]) {
    if (argc > 0) {
        throw std::invalid_argument(std::string("unknown option: ") + argv[0]);
    }
}

int Palette256Command::run(Terminal& terminal) const {
    const os::TerminalSize dimensions = terminal.drawingArea();
    if (dimensions.columns < chartColumns || dimensions.rows < chartRows) {
        char message[128];
        std::snprintf(message, sizeof(message),
                      "Terminal too small: the chart needs %dx%d, this one has %dx%d.\n",
                      chartColumns, chartRows, dimensions.columns, dimensions.rows);
        os::write(message);
        return 1;
    }

    terminal.beginFrame();

    writeHeading(terminal, "System", indexCellWidth);
    writeSectionGap(terminal);
    writeHeading(terminal, "Cube (r,g,b)", cubeWidth);
    writeSectionGap(terminal);
    writeHeading(terminal, "Greys", indexCellWidth);
    writeGreyGap(terminal);
    writeHeading(terminal, "Cube grey", cubeCellWidth);
    terminal.resetColor();
    terminal.writeText("\n");

    for (int row = 0; row < swatchRows; ++row) {
        // Every row starts with a newline, which also closes the blank line under the
        // headings. None follows the last row, since that would scroll the frame.
        terminal.resetColor();
        terminal.writeText("\n");

        writeSystemCell(terminal, row);
        writeSectionGap(terminal);

        if (row < cubeRows) {
            for (int column = 0; column < cubeColumns; ++column) {
                const int index = cubeIndex(row, column);
                writeSwatch(terminal, index, cubeText(index));
            }
        } else {
            writeBlank(terminal, cubeWidth);
        }
        writeSectionGap(terminal);

        writeSwatch(terminal, rampFirstIndex + row, indexText(rampFirstIndex + row));
        writeGreyGap(terminal);

        // Labelled with coordinates, like the cube itself, so it reads as the cube's
        // diagonal rather than as six unrelated palette slots.
        const int cubeGreyStep = cubeGreyForRow(row);
        if (cubeGreyStep >= 0) {
            writeSwatch(terminal, cubeGreyIndex(cubeGreyStep), cubeText(cubeGreyIndex(cubeGreyStep)));
        } else {
            writeBlank(terminal, cubeCellWidth);
        }
    }

    terminal.endFrame();
    return 0;
}
