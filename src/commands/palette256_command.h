#pragma once

#include "command.h"

// Renders the whole 256-color palette as a reference chart: the system colors, the
// flattened color cube, and the grey ramp, each cell labelled with its own index.
class Palette256Command final : public Command {
public:
	void parseArgs(int argc, char* argv[]) override;
	int run(Terminal& terminal) const override;
};
