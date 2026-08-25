#include "terminal_os.h"

// The POSIX backend. CMake compiles this file everywhere except Windows, so nothing
// here is conditional; the guard turns a wrong-platform build into one clear error
// instead of a wall of them.
#ifdef _WIN32
#error "terminal_os_posix.cpp is the POSIX backend; Windows builds terminal_os_windows.cpp."
#endif

#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>

namespace os {

TerminalSize terminalSize() {
    struct winsize window{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &window) == 0 && window.ws_col > 0 && window.ws_row > 0) {
        return {static_cast<int>(window.ws_col), static_cast<int>(window.ws_row)};
    }
    return {80, 24};
}

namespace {

termios savedInputMode{};
bool rawInput = false;

}  // namespace

void beginRawInput() {
    if (!isatty(STDIN_FILENO) || tcgetattr(STDIN_FILENO, &savedInputMode) != 0) {
        return;
    }
    termios raw = savedInputMode;
    // ISIG stays on so Ctrl-C keeps working; only echo and line assembly go away.
    raw.c_lflag &= ~(static_cast<tcflag_t>(ICANON) | static_cast<tcflag_t>(ECHO));
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    rawInput = tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0;
}

void endRawInput() {
    if (rawInput) {
        tcsetattr(STDIN_FILENO, TCSANOW, &savedInputMode);
        rawInput = false;
    }
}

std::size_t readTerminal(char* data, std::size_t size, int timeoutMilliseconds) {
    if (!rawInput) {
        return 0;
    }
    fd_set readable;
    FD_ZERO(&readable);
    FD_SET(STDIN_FILENO, &readable);
    timeval timeout{timeoutMilliseconds / 1000, (timeoutMilliseconds % 1000) * 1000};
    if (select(STDIN_FILENO + 1, &readable, nullptr, nullptr, &timeout) <= 0) {
        return 0;
    }
    const ssize_t count = read(STDIN_FILENO, data, size);
    return count > 0 ? static_cast<std::size_t>(count) : 0;
}

void prepareTerminal() {}
void restoreTerminal() {}

void sleepMilliseconds(int milliseconds) {
    if (milliseconds <= 0) {
        return;
    }
    struct timespec request{};
    request.tv_sec = milliseconds / 1000;
    request.tv_nsec = static_cast<long>(milliseconds % 1000) * 1000000L;
    while (nanosleep(&request, &request) == -1 && errno == EINTR) {
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
