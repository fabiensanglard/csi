#pragma once

#include "command.h"

class FillCommand final : public Command {
public:
	void parseArgs(int argc, char* argv[]) override;
	int run(Terminal& terminal) const override;

private:
	FillStyle fillStyle_ = FillStyle::Background;
	Rgb upperRight_ = {0, 0, 0};
	ColorMapping colorMapping_ = ColorMapping::All;
	// Only an explicit --mode is worth an error when the terminal has no palette.
	bool colorMappingSet_ = false;
};
