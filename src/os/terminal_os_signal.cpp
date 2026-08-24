#include "terminal_os.h"

// Ctrl-C handling. std::signal is standard C++ and behaves the same on both
// backends, so this file is compiled everywhere rather than duplicated per platform.
// One flag is shared by the whole program: a command that runs its own loop stops on
// the same interrupt that releases main, so Ctrl-C is only ever needed once.

#include <csignal>

namespace os {
namespace {

volatile std::sig_atomic_t interruptFlag = 0;

void onInterrupt(int) {
    interruptFlag = 1;
}

}  // namespace

void installInterruptHandler() {
    interruptFlag = 0;
    std::signal(SIGINT, onInterrupt);
}

bool interrupted() {
    return interruptFlag != 0;
}

void waitForInterrupt() {
    // Polling keeps this portable; the interval is short enough to feel immediate and
    // long enough that the wait costs nothing measurable.
    while (interruptFlag == 0) {
        sleepMilliseconds(50);
    }
}

}  // namespace os
