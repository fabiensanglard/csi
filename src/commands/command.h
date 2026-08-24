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

	// Whether main should hold the finished screen until Ctrl-C. True for a command
	// that paints one frame and returns, since the shell prompt would otherwise
	// scroll it away; false for one that runs its own loop and has already decided
	// to stop, where holding would demand a second Ctrl-C or hang out --seconds.
	virtual bool holdsScreen() const { return true; }
};

Rgb parseCornerColor(const std::string& value);
FillStyle parseFillStyle(const std::string& value);
void printUsage(Terminal& terminal, const char* program);
