#pragma once

#include "../os/terminal_os.h"

// Establishes that the terminal has finished processing a frame.
//
// The problem: writing to a terminal tells you nothing about when it drew. fwrite
// and fflush hand bytes to the kernel's tty buffer and return as soon as it accepts
// them, so timing a flush measures how full the pipe is, not the terminal's work.
//
// The fix is to ask a question the terminal cannot answer early. ESC[c requests the
// primary device attributes, and the terminal replies ESC[?...c. Because a terminal
// consumes its input stream strictly in order, the reply cannot be produced until
// everything queued ahead of the query has been parsed and applied. The interval
// between sending the query and seeing the reply is the time the terminal spent
// working through the frame.
//
// Weaker alternatives were tried and dropped: timing the flush alone reports only
// backpressure, and tcdrain proves the bytes were delivered but not that any were
// parsed. DSR (ESC[6n) works on the same ordering argument as ESC[c and measured the
// same; one query is enough. Wrapping frames in DEC mode 2026 makes a terminal
// present them atomically, which is a cure for tearing rather than a measurement,
// and Terminal.app does not implement it.
//
// What this proves is that the frame was processed, not that photons have landed:
// no terminal exposes a present or vsync acknowledgement, so compositing and display
// refresh sit outside the measurement.
//
// A terminal that never answers -- output redirected to a file, input that is not a
// tty, an emulator without device attributes -- would otherwise cost a full timeout
// on every frame, so the first silence retires the fence.
class Fence {
public:
    Fence();
    ~Fence();

    Fence(const Fence&) = delete;
    Fence& operator=(const Fence&) = delete;

    // Sends the query and blocks until the reply arrives. Returns the round trip in
    // microseconds, or -1 if the terminal did not answer within the timeout.
    long long wait(int timeoutMilliseconds = 250);

    bool supported() const { return supported_; }

private:
    bool supported_ = true;
};
