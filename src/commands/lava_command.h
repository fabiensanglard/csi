#pragma once

#include "command.h"

// Quake's turbulent surface effect: a static lava texture sampled through a
// sine warp that advances with time. Runs at 30 frames per second until
// interrupted, or until --seconds elapses.
class LavaCommand final : public Command {
public:
	void parseArgs(int argc, char* argv[]) override;
	int run(Terminal& terminal) const override;

private:
	FillStyle fillStyle_ = FillStyle::Background;
	double seconds_ = 0.0;  // 0 runs until Ctrl-C
};
