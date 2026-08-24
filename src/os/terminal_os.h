#pragma once

#include <cstddef>
#include <string>

namespace os {

struct TerminalSize {
    int columns;
    int rows;
};

TerminalSize terminalSize();
void prepareTerminal();
void restoreTerminal();
void sleepMilliseconds(int milliseconds);

// Ctrl-C handling, shared by the whole program. installInterruptHandler() makes
// Ctrl-C set a flag instead of killing the process; interrupted() reports whether it
// has arrived, and waitForInterrupt() blocks until it does.
void installInterruptHandler();
bool interrupted();
void waitForInterrupt();
void write(const std::string& text);
void write(const char* data, std::size_t size);

}  // namespace os
