#pragma once

#include "command.h"

#include "../terminal/fence.h"

// Quake's turbulent surface effect: a static lava texture sampled through a
// sine warp that advances with time. Runs at 30 frames per second until
// interrupted, or until --seconds elapses.
class LavaCommand final : public Command {
public:
	void parseArgs(int argc, char* argv[]) override;
	int run(Terminal& terminal) const override;
	bool holdsScreen() const override { return false; }

private:
	FillStyle fillStyle_ = FillStyle::Background;
	double seconds_ = 0.0;  // 0 runs until Ctrl-C
	// --no-stats gives a quiet run: no fence round trip, no summary.
	bool stats_ = true;
	// 0 removes the pacing entirely, so the loop runs as fast as the terminal will
	// take frames. That is the only way the readout measures the terminal rather
	// than the pacing.
	int framesPerSecond_ = 30;
};
