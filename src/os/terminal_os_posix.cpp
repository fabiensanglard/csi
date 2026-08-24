#include "terminal_os.h"

// The POSIX backend. CMake compiles this file everywhere except Windows, so nothing
// here is conditional; the guard turns a wrong-platform build into one clear error
// instead of a wall of them.
#ifdef _WIN32
#error "terminal_os_posix.cpp is the POSIX backend; Windows builds terminal_os_windows.cpp."
#endif

#include <sys/ioctl.h>
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
