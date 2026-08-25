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
// Raw terminal input, so a reply from the terminal can be read a byte at a time
// rather than waiting for a newline. Ctrl-C still raises SIGINT: only echo and line
// buffering are turned off. Both calls are no-ops when input is not a terminal.
void beginRawInput();
void endRawInput();
// Reads at most size bytes, waiting up to timeoutMilliseconds. Returns 0 on timeout.
std::size_t readTerminal(char* data, std::size_t size, int timeoutMilliseconds);

void installInterruptHandler();
bool interrupted();
void waitForInterrupt();
void write(const std::string& text);
void write(const char* data, std::size_t size);

}  // namespace os
