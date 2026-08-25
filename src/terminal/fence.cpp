#include "fence.h"

#include "csi.h"

#include <chrono>
#include <string>

Fence::Fence() {
    os::beginRawInput();
}

Fence::~Fence() {
    os::endRawInput();
}

long long Fence::wait(int timeoutMilliseconds) {
    if (!supported_) {
        return -1;
    }

    using Clock = std::chrono::steady_clock;
    const auto start = Clock::now();
    os::write(csi::primaryDeviceAttributes());

    // The reply ends at 'c'. Keystrokes typed while a frame is in flight land in the
    // same stream, so anything before the terminator is read and discarded.
    std::string reply;
    char buffer[64];
    for (;;) {
        const std::size_t count = os::readTerminal(buffer, sizeof(buffer), timeoutMilliseconds);
        if (count > 0) {
            reply.append(buffer, count);
            if (reply.find('c') != std::string::npos) {
                break;
            }
        }
        if (std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start).count()
            >= timeoutMilliseconds) {
            supported_ = false;
            return -1;
        }
    }

    return std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - start).count();
}
