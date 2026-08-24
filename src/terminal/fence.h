#pragma once

#include "../os/terminal_os.h"

// A round trip that establishes when the terminal has finished processing a frame.
//
// Writing to a terminal tells you nothing about when it drew: fwrite and fflush hand
// bytes to the kernel's tty buffer and return as soon as that buffer accepts them.
// The terminal may not have parsed a single byte yet. Timing a flush therefore
// measures backpressure -- how full the pipe is -- not the terminal's work.
//
// The fence closes that gap by asking a question the terminal cannot answer early.
// ESC[6n is the Device Status Report for cursor position; the terminal replies with
// ESC[row;colR. Because a terminal consumes its input stream strictly in order, the
// reply cannot be produced until everything queued ahead of the query has been
// parsed and applied. So the interval between sending ESC[6n and seeing the reply is
// the time the terminal spent working through the frame.
//
// What this proves is that the frame was *processed*, not that photons have landed:
// no terminal exposes a present or vsync acknowledgement, so compositing and display
// refresh sit outside the measurement.
//
// A terminal that never answers -- output redirected to a file, input that is not a
// tty, an emulator without DSR -- would otherwise cost a full timeout on every
// frame, so the first silence marks the fence unsupported and it stops asking.
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
