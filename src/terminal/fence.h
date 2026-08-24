#pragma once

#include "../os/terminal_os.h"

#include <string>

// Ways of establishing that a frame has actually landed, weakest to strongest.
//
// The problem: writing to a terminal tells you nothing about when it drew. fwrite
// and fflush hand bytes to the kernel's tty buffer and return as soon as it accepts
// them, so timing a flush measures how full the pipe is, not the terminal's work.
// None of these reach all the way to photons -- no terminal acknowledges a present,
// so compositing and display refresh are outside every one of them.
enum class FenceMode {
    // No measurement at all. The baseline: submit and move on.
    None,
    // Time fwrite plus fflush and nothing else. Reports backpressure only, and only
    // once the kernel buffer is full enough to start blocking.
    Flush,
    // tcdrain: block until every byte written has been handed to the terminal rather
    // than sitting in the kernel. Proves delivery, not that anything was parsed.
    Drain,
    // ESC[6n, cursor position report. The terminal answers ESC[row;colR, and because
    // it consumes its input strictly in order the reply cannot come back until
    // everything queued ahead of the query has been parsed and applied. Proves the
    // frame was processed.
    Dsr,
    // ESC[c, primary device attributes, answered ESC[?...c. Same ordering argument as
    // DSR; offered because terminals differ in which queries they answer and how
    // eagerly, so the two can disagree.
    Da,
    // Frame wrapped in DEC mode 2026 so the terminal presents it atomically, then
    // fenced with DSR. The wrapping removes tearing and lets the terminal skip
    // intermediate repaints; the DSR still only proves parsing, and it must come
    // after the closing sequence or it would time a frame whose present is still
    // deliberately pending.
    Sync
};

// Throws std::invalid_argument on an unknown name.
FenceMode parseFenceMode(const std::string& value);
const char* fenceModeName(FenceMode mode);

class Fence {
public:
    explicit Fence(FenceMode mode);
    ~Fence();

    Fence(const Fence&) = delete;
    Fence& operator=(const Fence&) = delete;

    // Runs whatever this mode does after a frame has been flushed. Returns the wait
    // in microseconds, or -1 when the mode measures nothing or the terminal did not
    // answer. A terminal that stays silent would otherwise cost a full timeout every
    // frame, so the first silence retires the fence.
    long long wait(int timeoutMilliseconds = 250);

    bool supported() const { return supported_; }
    FenceMode mode() const { return mode_; }

private:
    FenceMode mode_;
    bool supported_ = true;
};
