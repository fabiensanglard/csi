#pragma once

#include "terminal.h"

#include <cstdint>
#include <unordered_map>

class Terminal256 final : public Terminal {
public:
    explicit Terminal256(ColorMapping colorMapping = ColorMapping::All);
    bool setColorMapping(ColorMapping mapping) override;
    void setTextColor(const Rgb& color) override;
    void setBGColor(const Rgb& color) override;

private:
    Color256 map(const Rgb& color) const;

    ColorMapping colorMapping_;
    // Nearest mapping walks the whole palette, which is too slow to redo for every
    // cell of every animation frame; a frame reuses a handful of colors.
    mutable std::unordered_map<std::uint32_t, std::uint8_t> nearestCache_;
};
