#include "terminal_os.h"

// The Windows backend. CMake compiles this file only when WIN32 and the POSIX file
// everywhere else, so nothing here is conditional; the guard turns a wrong-platform
// build into one clear error instead of a wall of them.
#ifndef _WIN32
#error "terminal_os_windows.cpp is the Windows backend; other platforms build terminal_os_posix.cpp."
#endif

#include <windows.h>

#include <cstdio>

namespace os {
namespace {

DWORD originalMode = 0;
HANDLE outputHandle = INVALID_HANDLE_VALUE;

}  // namespace

TerminalSize terminalSize() {
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info) != 0) {
        return {info.srWindow.Right - info.srWindow.Left + 1,
                info.srWindow.Bottom - info.srWindow.Top + 1};
    }
    return {80, 24};
}

void prepareTerminal() {
    outputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (GetConsoleMode(outputHandle, &originalMode) != 0) {
        SetConsoleMode(outputHandle, originalMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
}

void restoreTerminal() {
    if (outputHandle != INVALID_HANDLE_VALUE) {
        SetConsoleMode(outputHandle, originalMode);
    }
}

void sleepMilliseconds(int milliseconds) {
    if (milliseconds > 0) {
        Sleep(static_cast<DWORD>(milliseconds));
    }
}

void write(const std::string& text) {
    std::fwrite(text.data(), 1, text.size(), stdout);
    std::fflush(stdout);
}

void write(const char* data, std::size_t size) {
    std::fwrite(data, 1, size, stdout);
    std::fflush(stdout);
}

}  // namespace os
