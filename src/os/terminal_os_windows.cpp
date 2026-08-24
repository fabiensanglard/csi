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

namespace {

DWORD savedInputMode = 0;
HANDLE inputHandle = INVALID_HANDLE_VALUE;
bool rawInput = false;

}  // namespace

void beginRawInput() {
    inputHandle = GetStdHandle(STD_INPUT_HANDLE);
    if (GetConsoleMode(inputHandle, &savedInputMode) == 0) {
        return;
    }
    DWORD mode = savedInputMode & ~static_cast<DWORD>(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
    mode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
    rawInput = SetConsoleMode(inputHandle, mode) != 0;
}

void endRawInput() {
    if (rawInput) {
        SetConsoleMode(inputHandle, savedInputMode);
        rawInput = false;
    }
}

std::size_t readTerminal(char* data, std::size_t size, int timeoutMilliseconds) {
    if (!rawInput) {
        return 0;
    }
    // The handle signals on any console event, not only on typed characters, so a
    // wake with nothing to read is treated the same as a timeout.
    if (WaitForSingleObject(inputHandle, static_cast<DWORD>(timeoutMilliseconds)) != WAIT_OBJECT_0) {
        return 0;
    }
    DWORD count = 0;
    if (ReadFile(inputHandle, data, static_cast<DWORD>(size), &count, nullptr) == 0) {
        return 0;
    }
    return count;
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
