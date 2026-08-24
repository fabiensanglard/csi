#include "fence.h"

#include "csi.h"

#include <chrono>
#include <stdexcept>

namespace {

using Clock = std::chrono::steady_clock;

long long microsSince(Clock::time_point start) {
    return std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - start).count();
}

}  // namespace

FenceMode parseFenceMode(const std::string& value) {
    if (value == "none") {
        return FenceMode::None;
    }
    if (value == "flush") {
        return FenceMode::Flush;
    }
    if (value == "drain") {
        return FenceMode::Drain;
    }
    if (value == "dsr") {
        return FenceMode::Dsr;
    }
    if (value == "da") {
        return FenceMode::Da;
    }
    if (value == "sync") {
        return FenceMode::Sync;
    }
    throw std::invalid_argument("--stats must be none, flush, drain, dsr, da, or sync");
}

const char* fenceModeName(FenceMode mode) {
    switch (mode) {
    case FenceMode::None:
        return "none";
    case FenceMode::Flush:
        return "flush";
    case FenceMode::Drain:
        return "drain";
    case FenceMode::Dsr:
        return "dsr";
    case FenceMode::Da:
        return "da";
    case FenceMode::Sync:
        return "sync";
    }
    return "none";
}

Fence::Fence(FenceMode mode) : mode_(mode) {
    if (mode_ == FenceMode::Dsr || mode_ == FenceMode::Da || mode_ == FenceMode::Sync) {
        os::beginRawInput();
    }
}

Fence::~Fence() {
    os::endRawInput();
}

long long Fence::wait(int timeoutMilliseconds) {
    if (!supported_ || mode_ == FenceMode::None || mode_ == FenceMode::Flush) {
        return -1;
    }

    const auto start = Clock::now();

    if (mode_ == FenceMode::Drain) {
        os::drainOutput();
        return microsSince(start);
    }

    // DSR closes on 'R', DA on 'c'. Sync fences with DSR: the mode 2026 wrapping is
    // emitted by the terminal itself, around the frame, and has already closed by the
    // time this runs.
    const bool attributes = mode_ == FenceMode::Da;
    const char terminator = attributes ? 'c' : 'R';
    os::write(attributes ? csi::primaryDeviceAttributes() : csi::deviceStatusReport());

    // Keystrokes typed while a frame is in flight arrive in the same stream, so
    // anything ahead of the terminator is read and discarded.
    std::string reply;
    char buffer[64];
    for (;;) {
        const std::size_t count = os::readTerminal(buffer, sizeof(buffer), timeoutMilliseconds);
        if (count > 0) {
            reply.append(buffer, count);
            if (reply.find(terminator) != std::string::npos) {
                break;
            }
        }
        if (std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start).count()
            >= timeoutMilliseconds) {
            supported_ = false;
            return -1;
        }
    }

    return microsSince(start);
}
