#include "lava_command.h"

#include "../terminal/fence.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <vector>

namespace {

// Quake's turbulence constants, from the software renderer: a 64x64 texture and a
// sine table of CYCLE entries per period. Quake walked the table at SPEED = 20
// entries per second, which at cell resolution is a barely moving surface; this
// rolls visibly and still moves less than one cell per frame at 30 fps.
constexpr int textureSize = 64;
constexpr int textureMask = textureSize - 1;
constexpr int cycle = 128;
constexpr double speed = 43.74;

// Quake's AMP, a little over: the warp displaces the surface by up to 10.16 texels.
constexpr double amplitude = 10.164;

// Texels per cell. Cells are about twice as tall as they are wide, so a row covers
// twice the texture a column does and the lava keeps its proportions.
constexpr double texelsPerColumn = 1.0;
constexpr double texelsPerRow = 2.0;


// One period, plus a repeat of the first entry so a fractional read never runs off
// the end. Quake read this table at integer positions in 16.16 fixed point; at cell
// resolution that quantization is visible as bands snapping a whole cell sideways,
// so positions here are fractional and the table is interpolated. The constant bias
// Quake added to keep its fixed-point math positive only shifts the whole surface,
// and is dropped.
using SineTable = std::array<double, cycle + 1>;

SineTable buildSineTable() {
    SineTable table{};
    for (std::size_t index = 0; index < table.size(); ++index) {
        const double angle = static_cast<double>(index) * 2.0 * 3.14159265358979323846 / cycle;
        table[index] = std::sin(angle) * amplitude;
    }
    return table;
}

// Displacement in texels at a fractional position along the table.
double turbulence(const SineTable& table, double position) {
    double wrapped = std::fmod(position, static_cast<double>(cycle));
    if (wrapped < 0.0) {
        wrapped += cycle;
    }
    const int index = static_cast<int>(wrapped);
    const double fraction = wrapped - index;
    return table[index] * (1.0 - fraction) + table[index + 1] * fraction;
}

// The color the lava reaches at a given heat, from crusted rock to white hot.
struct RampStop {
    double position;
    Rgb color;
};

constexpr RampStop rampStops[] = {
    {0.00, {40, 6, 2}},
    {0.30, {112, 18, 4}},
    {0.55, {192, 48, 8}},
    {0.75, {232, 108, 16}},
    {0.90, {252, 176, 40}},
    {1.00, {255, 240, 168}}
};

using Ramp = std::array<Rgb, 256>;

Ramp buildRamp() {
    constexpr int stopCount = static_cast<int>(sizeof(rampStops) / sizeof(rampStops[0]));
    Ramp ramp{};
    for (int index = 0; index < static_cast<int>(ramp.size()); ++index) {
        const double position = static_cast<double>(index) / (ramp.size() - 1);
        int stop = 0;
        while (stop < stopCount - 2 && position > rampStops[stop + 1].position) {
            ++stop;
        }
        const RampStop& from = rampStops[stop];
        const RampStop& to = rampStops[stop + 1];
        const double span = to.position - from.position;
        const double fraction = span <= 0.0 ? 0.0 : (position - from.position) / span;
        ramp[index] = interpolate(from.color, to.color, fraction);
    }
    return ramp;
}

// Value noise on a torus: lattice coordinates wrap at the frequency, so every
// octave tiles and the finished texture has no seam where the warp wraps it.
std::uint32_t hashLattice(int x, int y, int frequency) {
    std::uint32_t value = static_cast<std::uint32_t>(x) * 374761393u
                        + static_cast<std::uint32_t>(y) * 668265263u
                        + static_cast<std::uint32_t>(frequency) * 2246822519u
                        + 0x9E3779B9u;
    value ^= value >> 13;
    value *= 1274126177u;
    value ^= value >> 16;
    return value;
}

double latticeValue(int x, int y, int frequency) {
    return hashLattice(x & (frequency - 1), y & (frequency - 1), frequency) / 4294967295.0;
}

double noise(double horizontal, double vertical, int frequency) {
    const double x = horizontal * frequency;
    const double y = vertical * frequency;
    const int x0 = static_cast<int>(std::floor(x));
    const int y0 = static_cast<int>(std::floor(y));
    const auto smooth = [](double value) { return value * value * (3.0 - 2.0 * value); };
    const double weightX = smooth(x - x0);
    const double weightY = smooth(y - y0);
    const double top = latticeValue(x0, y0, frequency) * (1.0 - weightX)
                     + latticeValue(x0 + 1, y0, frequency) * weightX;
    const double bottom = latticeValue(x0, y0 + 1, frequency) * (1.0 - weightX)
                        + latticeValue(x0 + 1, y0 + 1, frequency) * weightX;
    return top * (1.0 - weightY) + bottom * weightY;
}

using Texture = std::array<std::uint8_t, textureSize * textureSize>;

// A crust of cooled rock split by bright veins. The veins are the contour where the
// noise crosses its midpoint, which reads as cracks rather than blobs.
Texture buildTexture() {
    std::array<double, textureSize * textureSize> field{};
    double lowest = 1.0;
    double highest = 0.0;
    for (int y = 0; y < textureSize; ++y) {
        for (int x = 0; x < textureSize; ++x) {
            const double horizontal = static_cast<double>(x) / textureSize;
            const double vertical = static_cast<double>(y) / textureSize;
            const double value = noise(horizontal, vertical, 4)
                               + noise(horizontal, vertical, 8) * 0.5
                               + noise(horizontal, vertical, 16) * 0.25
                               + noise(horizontal, vertical, 32) * 0.125;
            const double normalized = value / 1.875;
            field[y * textureSize + x] = normalized;
            lowest = std::min(lowest, normalized);
            highest = std::max(highest, normalized);
        }
    }

    const double span = highest - lowest;
    Texture texture{};
    for (std::size_t index = 0; index < field.size(); ++index) {
        const double normalized = span <= 0.0 ? 0.0 : (field[index] - lowest) / span;
        const double vein = 1.0 - std::fabs(normalized * 2.0 - 1.0);
        const double heat = std::clamp(0.30 * normalized + 0.75 * std::pow(vein, 6.0), 0.0, 1.0);
        texture[index] = static_cast<std::uint8_t>(std::lround(heat * 255.0));
    }
    return texture;
}

// Bilinear fetch with wrap. Quake could take the nearest texel because a pixel was
// about a texel wide; a cell covers enough of the texture that nearest sampling
// shows the warp as blocks stepping from one texel to the next.
double sampleTexture(const Texture& texture, double s, double t) {
    const double wrappedS = s - std::floor(s / textureSize) * textureSize;
    const double wrappedT = t - std::floor(t / textureSize) * textureSize;
    const int s0 = static_cast<int>(wrappedS) & textureMask;
    const int t0 = static_cast<int>(wrappedT) & textureMask;
    const int s1 = (s0 + 1) & textureMask;
    const int t1 = (t0 + 1) & textureMask;
    const double fractionS = wrappedS - std::floor(wrappedS);
    const double fractionT = wrappedT - std::floor(wrappedT);
    const double top = texture[(t0 << 6) + s0] * (1.0 - fractionS) + texture[(t0 << 6) + s1] * fractionS;
    const double bottom = texture[(t1 << 6) + s0] * (1.0 - fractionS) + texture[(t1 << 6) + s1] * fractionS;
    return top * (1.0 - fractionT) + bottom * fractionT;
}

}  // namespace

void LavaCommand::parseArgs(int argc, char* argv[]) {
    for (int argumentIndex = 0; argumentIndex < argc; ++argumentIndex) {
        const std::string argument = argv[argumentIndex];
        if (argument == "--no-stats") {
            fenceMode_ = FenceMode::None;
            continue;
        }
        if (argument != "--fill" && argument != "--seconds" && argument != "--fps"
            && argument != "--stats") {
            throw std::invalid_argument("unknown option: " + argument);
        }
        if (argumentIndex + 1 >= argc) {
            throw std::invalid_argument("missing value for " + argument);
        }
        const std::string value = argv[++argumentIndex];
        if (argument == "--fill") {
            fillStyle_ = parseFillStyle(value);
        } else if (argument == "--stats") {
            fenceMode_ = parseFenceMode(value);
        } else if (argument == "--fps") {
            try {
                framesPerSecond_ = std::stoi(value);
            } catch (const std::exception&) {
                throw std::invalid_argument("--fps must be a whole number of frames");
            }
            if (framesPerSecond_ < 0) {
                throw std::invalid_argument("--fps must not be negative");
            }
        } else {
            try {
                seconds_ = std::stod(value);
            } catch (const std::exception&) {
                throw std::invalid_argument("--seconds must be a number of seconds");
            }
            if (seconds_ < 0.0) {
                throw std::invalid_argument("--seconds must not be negative");
            }
        }
    }
}

int LavaCommand::run(Terminal& terminal) const {
    os::TerminalSize dimensions = terminal.drawingArea();
    if (dimensions.columns <= 0 || dimensions.rows <= 0) {
        os::write("Unable to determine terminal size.\n");
        return 1;
    }

    const SineTable sineTable = buildSineTable();
    const Ramp ramp = buildRamp();
    const Texture texture = buildTexture();
    const bool block = fillStyle_ == FillStyle::Block;

    // Each axis is displaced by the turbulence read at the other axis, so the offset
    // for a cell depends on its row and its column separately: one lookup per row and
    // one per column covers the whole frame.
    std::vector<double> columnShift;
    std::vector<double> rowShift;

    using Clock = std::chrono::steady_clock;
    // 0 means uncapped: no deadline, no sleep, submit as fast as the terminal takes.
    const bool paced = framesPerSecond_ > 0;
    const auto frameDuration = std::chrono::microseconds(paced ? 1000000 / framesPerSecond_ : 0);
    const auto start = Clock::now();
    auto deadline = start;

    // Three costs, measured apart. Build is our own work. Flush is fwrite plus
    // fflush, which only reports backpressure: it returns once the kernel accepts
    // the bytes, whether or not the terminal has looked at them. The fence is the
    // honest one -- a round trip the terminal cannot answer until it has processed
    // the frame. The sleep that paces the loop is outside all three.
    const bool measuring = fenceMode_ != FenceMode::None;
    Fence fence(fenceMode_);
    terminal.setSynchronized(fenceMode_ == FenceMode::Sync);
    long long frames = 0;
    long long buildMicros = 0;
    long long flushMicros = 0;
    long long fenceMicros = 0;
    long long fenceFrames = 0;
    long long buildMax = 0;
    long long flushMax = 0;
    long long fenceMax = 0;
    unsigned long long bytes = 0;

    // Smoothed so the readout is legible rather than flickering every frame.
    double displayedFps = 0.0;
    auto previousFrame = Clock::now();

    bool cleared = false;
    while (!os::interrupted()) {
        const auto frameStart = Clock::now();
        const double elapsed = std::chrono::duration<double>(Clock::now() - start).count();
        if (seconds_ > 0.0 && elapsed >= seconds_) {
            break;
        }

        // A resized window leaves stale cells outside the new bounds, so clear it and
        // start over; otherwise repaint in place.
        const os::TerminalSize current = terminal.drawingArea();
        if (!cleared || current.columns != dimensions.columns || current.rows != dimensions.rows) {
            dimensions = current;
            cleared = true;
            terminal.beginFrame();
        } else {
            terminal.beginRepaint();
        }

        // Quake reads the table from a phase that advances with time, and displaces
        // each axis by the turbulence at the other one. That cross-coupling is what
        // makes the surface roll instead of merely sliding.
        const double phase = elapsed * speed;
        columnShift.resize(static_cast<std::size_t>(dimensions.columns));
        for (int column = 0; column < dimensions.columns; ++column) {
            columnShift[column] = turbulence(sineTable, phase + column * texelsPerColumn);
        }
        rowShift.resize(static_cast<std::size_t>(dimensions.rows));
        for (int row = 0; row < dimensions.rows; ++row) {
            rowShift[row] = turbulence(sineTable, phase + row * texelsPerRow);
        }

        for (int row = 0; row < dimensions.rows; ++row) {
            if (row > 0) {
                terminal.resetColor();
                terminal.writeText("\n");
            }
            const double t = row * texelsPerRow;
            for (int column = 0; column < dimensions.columns; ++column) {
                const double s = column * texelsPerColumn;
                const double heat = sampleTexture(texture, s + rowShift[row], t + columnShift[column]);
                const Rgb color = ramp[static_cast<std::size_t>(std::lround(heat))];
                if (block) {
                    terminal.setTextColor(color);
                } else {
                    terminal.setBGColor(color);
                }
                terminal.writeText(block ? blockGlyph : " ");
            }
        }
        // Just the rate, so the corner stays readable over the animation. The
        // timing breakdown belongs in the summary printed on exit.
        char overlay[32];
        if (displayedFps <= 0.0) {
            // No interval to measure yet on the very first frame.
            std::snprintf(overlay, sizeof(overlay), " -- fps ");
        } else {
            std::snprintf(overlay, sizeof(overlay), " %.1f fps ", displayedFps);
        }
        terminal.writeText(csi::cursorTo(1, 1));
        terminal.setBGColor({0, 0, 0});
        terminal.setTextColor({255, 255, 255});
        terminal.writeText(overlay);
        terminal.resetColor();

        const auto flushStart = Clock::now();
        const std::size_t written = terminal.commit();
        const auto flushEnd = Clock::now();

        const long long fenced = fence.wait();

        const auto build = std::chrono::duration_cast<std::chrono::microseconds>(flushStart - frameStart).count();
        const auto flush = std::chrono::duration_cast<std::chrono::microseconds>(flushEnd - flushStart).count();
        ++frames;
        buildMicros += build;
        flushMicros += flush;
        buildMax = build > buildMax ? build : buildMax;
        flushMax = flush > flushMax ? flush : flushMax;
        bytes += written;
        if (fenced >= 0) {
            ++fenceFrames;
            fenceMicros += fenced;
            fenceMax = fenced > fenceMax ? fenced : fenceMax;
        }

        // Rate over the whole frame, pacing sleep included, which is what the eye
        // actually sees. A tenth weight settles in well under a second at 30 fps.
        const auto frameEnd = Clock::now();
        const double delta = std::chrono::duration<double>(frameEnd - previousFrame).count();
        previousFrame = frameEnd;
        if (delta > 0.0) {
            const double instant = 1.0 / delta;
            displayedFps = displayedFps > 0.0 ? displayedFps * 0.9 + instant * 0.1 : instant;
        }

        // The deadline is absolute, so a frame that sleeps a little long is paid back
        // by the next one instead of dragging the whole animation behind. Sleeping
        // rounds down to whole milliseconds and the last fraction is simply dropped:
        // waking early costs nothing, waking late costs a frame.
        if (!paced) {
            continue;
        }
        deadline += frameDuration;
        auto now = Clock::now();
        if (now > deadline + frameDuration) {
            deadline = now;
        }
        while (now < deadline) {
            const auto remaining = std::chrono::duration_cast<std::chrono::microseconds>(deadline - now).count();
            if (remaining < 1000) {
                break;
            }
            os::sleepMilliseconds(static_cast<int>(remaining / 1000));
            now = Clock::now();
        }
    }
    terminal.endFrame();

    if (measuring) {
        const double wall = std::chrono::duration<double>(Clock::now() - start).count();
        const double perFrame = frames > 0 ? static_cast<double>(frames) : 1.0;
        char report[640];
        std::snprintf(report, sizeof(report),
                      "frames    %lld in %.2f s\n"
                      "achieved  %.1f fps (target %d, 0 = uncapped)\n"
                      "build     %.2f ms avg, %.2f ms max\n"
                      "flush     %.2f ms avg, %.2f ms max  (backpressure only)\n"
                      "output    %.0f bytes/frame, %.2f MB/s\n",
                      frames, wall,
                      wall > 0.0 ? frames / wall : 0.0, framesPerSecond_,
                      buildMicros / perFrame / 1000.0, buildMax / 1000.0,
                      flushMicros / perFrame / 1000.0, flushMax / 1000.0,
                      static_cast<double>(bytes) / perFrame,
                      wall > 0.0 ? static_cast<double>(bytes) / wall / 1e6 : 0.0);
        os::write(report);

        char fenceReport[256];
        if (fenceFrames > 0) {
            std::snprintf(fenceReport, sizeof(fenceReport),
                          "fence     %.2f ms avg, %.2f ms max over %lld frames (%s)\n",
                          fenceMicros / static_cast<double>(fenceFrames) / 1000.0,
                          fenceMax / 1000.0, fenceFrames, fenceModeName(fenceMode_));
        } else if (fenceMode_ == FenceMode::Flush) {
            std::snprintf(fenceReport, sizeof(fenceReport),
                          "fence     flush only, no round trip requested\n");
        } else {
            std::snprintf(fenceReport, sizeof(fenceReport),
                          "fence     %s unavailable: the terminal did not answer\n",
                          fenceModeName(fenceMode_));
        }
        os::write(fenceReport);
    }
    return 0;
}
