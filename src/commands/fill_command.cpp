#include "fill_command.h"

#include <stdexcept>

void FillCommand::parseArgs(int argc, char* argv[]) {
    for (int argumentIndex = 0; argumentIndex < argc; ++argumentIndex) {
        const std::string argument = argv[argumentIndex];
        if (argument != "--color" && argument != "--fill" && argument != "--mode") {
            throw std::invalid_argument("unknown option: " + argument);
        }
        if (argumentIndex + 1 >= argc) {
            throw std::invalid_argument("missing value for " + argument);
        }
        const std::string value = argv[++argumentIndex];
        if (argument == "--color") {
            upperRight_ = parseCornerColor(value);
        } else if (argument == "--mode") {
            colorMapping_ = parseColorMapping(value);
            colorMappingSet_ = true;
        } else {
            fillStyle_ = parseFillStyle(value);
        }
    }
}

int FillCommand::run(Terminal& terminal) const {
    if (!terminal.setColorMapping(colorMapping_) && colorMappingSet_) {
        throw std::invalid_argument("--mode only applies to --space 256");
    }

    const os::TerminalSize dimensions = terminal.drawingArea();
    if (dimensions.columns <= 0 || dimensions.rows <= 0) {
        os::write("Unable to determine terminal size.\n");
        return 1;
    }

    const bool block = fillStyle_ == FillStyle::Block;
    terminal.beginFrame();
    for (int row = 0; row < dimensions.rows; ++row) {
        // Between rows only: a newline after the last one would scroll the frame.
        if (row > 0) {
            terminal.resetColor();
            terminal.writeText("\n");
        }
        const double vertical = dimensions.rows == 1 ? 0.0 : static_cast<double>(row) / (dimensions.rows - 1);
        for (int column = 0; column < dimensions.columns; ++column) {
            const double horizontal = dimensions.columns == 1 ? 0.0 : static_cast<double>(column) / (dimensions.columns - 1);
            const Rgb color = bilinearInterpolate({255, 0, 0}, upperRight_,
                                                  {0, 255, 0}, {0, 0, 255},
                                                  horizontal, vertical);
            if (block) {
                terminal.setTextColor(color);
            } else {
                terminal.setBGColor(color);
            }
            terminal.writeText(block ? blockGlyph : " ");
        }
    }
    terminal.endFrame();
    return 0;
}
