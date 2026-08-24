#include "terminal256.h"

#include <climits>

namespace {

// The 216-color cube occupies slots 16-231, one step per level pair.
constexpr int levels[] = {0, 95, 135, 175, 215, 255};

// One search, three candidate sets: the modes differ only in which entries they may
// pick, never in how the match is made.
Color256 nearest(const Rgb& color, ColorMapping mapping) {
    int bestIndex = 0;
    int bestDistance = INT_MAX;
    const auto consider = [&](int index, const Rgb& candidate) {
        const int red = static_cast<int>(color.red) - candidate.red;
        const int green = static_cast<int>(color.green) - candidate.green;
        const int blue = static_cast<int>(color.blue) - candidate.blue;
        const int distance = red * red + green * green + blue * blue;
        if (distance < bestDistance) {
            bestIndex = index;
            bestDistance = distance;
        }
    };

    // Only All gambles on the system palette. Those sixteen are whatever the theme
    // paints them, so the values here are a guess; the other two modes skip them.
    if (mapping == ColorMapping::All) {
        for (int index = 0; index < csi::paletteSize; ++index) {
            const csi::PaletteEntry& entry = csi::palette[index];
            consider(index, {entry.red, entry.green, entry.blue});
        }
    }

    for (int red = 0; red < 6; ++red) {
        for (int green = 0; green < 6; ++green) {
            for (int blue = 0; blue < 6; ++blue) {
                consider(16 + 36 * red + 6 * green + blue,
                         {static_cast<std::uint8_t>(levels[red]), static_cast<std::uint8_t>(levels[green]),
                          static_cast<std::uint8_t>(levels[blue])});
            }
        }
    }

    // The cube's own neutral diagonal has just six steps, so the ramp is what keeps
    // near-grey gradients smooth.
    if (mapping != ColorMapping::Cube) {
        for (int gray = 0; gray < 24; ++gray) {
            const auto level = static_cast<std::uint8_t>(8 + gray * 10);
            consider(232 + gray, {level, level, level});
        }
    }

    return {static_cast<std::uint8_t>(bestIndex)};
}

}  // namespace

Terminal256::Terminal256(ColorMapping colorMapping) : Terminal(), colorMapping_(colorMapping) {}

bool Terminal256::setColorMapping(ColorMapping mapping) {
    // Cached matches were chosen from the old candidate set, so they cannot be
    // trusted once the set changes.
    colorMapping_ = mapping;
    nearestCache_.clear();
    return true;
}

Color256 Terminal256::map(const Rgb& color) const {
    const std::uint32_t key = (static_cast<std::uint32_t>(color.red) << 16)
                            | (static_cast<std::uint32_t>(color.green) << 8)
                            | color.blue;
    const auto cached = nearestCache_.find(key);
    if (cached != nearestCache_.end()) {
        return {cached->second};
    }
    const Color256 mapped = nearest(color, colorMapping_);
    nearestCache_.emplace(key, mapped.value);
    return mapped;
}

void Terminal256::setTextColor(const Rgb& color) {
    appendColorSequence(csi::color256(map(color), csi::Layer::Foreground));
}

void Terminal256::setBGColor(const Rgb& color) {
    appendColorSequence(csi::color256(map(color), csi::Layer::Background));
}
