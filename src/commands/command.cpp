#include "command.h"

#include <stdexcept>

Rgb parseCornerColor(const std::string& value) {
    if (value == "black") {
        return {0, 0, 0};
    }
    if (value == "white") {
        return {255, 255, 255};
    }
    if (value == "red") {
        return {255, 0, 0};
    }
    if (value == "green") {
        return {0, 255, 0};
    }
    if (value == "blue") {
        return {0, 0, 255};
    }
    if (value.size() == 7 && value[0] == '#') {
        const auto hexValue = [](char character) {
            if (character >= '0' && character <= '9') {
                return character - '0';
            }
            if (character >= 'a' && character <= 'f') {
                return character - 'a' + 10;
            }
            if (character >= 'A' && character <= 'F') {
                return character - 'A' + 10;
            }
            return -1;
        };
        const auto component = [&hexValue](char high, char low) {
            const int highValue = hexValue(high);
            const int lowValue = hexValue(low);
            if (highValue < 0 || lowValue < 0) {
                throw std::invalid_argument("--color must be a named color or #RRGGBB");
            }
            return static_cast<std::uint8_t>(highValue * 16 + lowValue);
        };
        return {component(value[1], value[2]), component(value[3], value[4]), component(value[5], value[6])};
    }
    throw std::invalid_argument("--color must be black, white, red, green, blue, or #RRGGBB");
}

FillStyle parseFillStyle(const std::string& value) {
    if (value == "bg") {
        return FillStyle::Background;
    }
    if (value == "c") {
        return FillStyle::Block;
    }
    throw std::invalid_argument("--fill must be c or bg");
}

void printUsage(Terminal& terminal, const char* program) {
    terminal.setBold(true);
    terminal.writeText("Usage:");
    terminal.setBold(false);
    terminal.writeText(std::string(" ") + program + " <command> [options]\n\n");

    terminal.setBold(true);
    terminal.writeText("Commands:\n");
    terminal.setBold(false);
    terminal.setTextColor({0, 255, 0});
    terminal.writeText("  fill");
    terminal.writeText("       Render the gradient\n");
    terminal.setTextColor({0, 255, 0});
    terminal.writeText("  lava");
    terminal.writeText("       Animate Quake lava at 30 fps\n");
    terminal.setTextColor({0, 255, 0});
    terminal.writeText("  256");
    terminal.writeText("        Chart the 256-color palette with each index\n\n");

    terminal.setBold(true);
    terminal.writeText("fill options:\n");
    terminal.setBold(false);
    terminal.setTextColor({255, 255, 0});
    terminal.writeText("  --color");
    terminal.writeText("  <name|#RRGGBB>     Upper-right corner color\n");
    terminal.setTextColor({255, 255, 0});
    terminal.writeText("  --fill");
    terminal.writeText("   <c|bg>             Fill glyph or background\n");
    terminal.setTextColor({255, 255, 0});
    terminal.writeText("  --mode");
    terminal.writeText("   <all|known|cube>   Palette subset (--space 256 only)\n\n");

    terminal.setBold(true);
    terminal.writeText("lava options:\n");
    terminal.setBold(false);
    terminal.setTextColor({255, 255, 0});
    terminal.writeText("  --fill");
    terminal.writeText("   <c|bg>             Fill glyph or background\n");
    terminal.setTextColor({255, 255, 0});
    terminal.writeText("  --seconds");
    terminal.writeText(" <n>               Stop after n seconds (default: Ctrl-C)\n");
    terminal.setTextColor({255, 255, 0});
    terminal.writeText("  --stats");
    terminal.writeText("                     Show frame rate and report timing\n\n");

    terminal.setBold(true);
    terminal.writeText("Global options:\n");
    terminal.setBold(false);
    terminal.setTextColor({255, 255, 0});
    terminal.writeText("  --space");
    terminal.writeText("  <16|256|RGB>       Terminal color space\n");
    terminal.setTextColor({255, 255, 0});
    terminal.writeText("  -h, --help");
    terminal.writeText("                  Show this help\n");
    terminal.setTextColor({255, 255, 0});
    terminal.writeText("  -v, --version");
    terminal.writeText("               Show version\n");
    terminal.resetColor();
    terminal.commit();
}
