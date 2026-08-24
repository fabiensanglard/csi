#pragma once

#include "../terminal/terminal.h"

#include <string>

// How a command paints a cell: color the glyph, or color the cell behind it.
enum class FillStyle {
	Background,
	Block
};

// U+2588 FULL BLOCK, painted in the foreground color.
constexpr const char* blockGlyph = "\xE2\x96\x88";

class Command {
public:
	virtual ~Command() = default;

	virtual void parseArgs(int argc, char* argv[]) = 0;
	virtual int run(Terminal& terminal) const = 0;
};

Rgb parseCornerColor(const std::string& value);
FillStyle parseFillStyle(const std::string& value);
void printUsage(Terminal& terminal, const char* program);
