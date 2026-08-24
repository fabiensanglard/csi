#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

struct Rgb {
    std::uint8_t red;
    std::uint8_t green;
    std::uint8_t blue;
};

struct Color256 {
    std::uint8_t value;
};

struct Color16 {
    std::uint8_t value;
};

inline Rgb interpolate(const Rgb& from, const Rgb& to, double fraction) {
    const auto channel = [fraction](std::uint8_t fromValue, std::uint8_t toValue) {
        const double value = fromValue + (static_cast<double>(toValue) - fromValue) * fraction;
        return static_cast<std::uint8_t>(std::clamp(std::lround(value), 0L, 255L));
    };
    return {channel(from.red, to.red), channel(from.green, to.green), channel(from.blue, to.blue)};
}

inline Rgb bilinearInterpolate(const Rgb& topLeft, const Rgb& topRight,
                               const Rgb& bottomLeft, const Rgb& bottomRight,
                               double horizontal, double vertical) {
    const double topLeftWeight = (1.0 - horizontal) * (1.0 - vertical);
    const double topRightWeight = horizontal * (1.0 - vertical);
    const double bottomLeftWeight = (1.0 - horizontal) * vertical;
    const double bottomRightWeight = horizontal * vertical;
    const auto channel = [topLeftWeight, topRightWeight, bottomLeftWeight, bottomRightWeight](
                             std::uint8_t topLeftValue, std::uint8_t topRightValue,
                             std::uint8_t bottomLeftValue, std::uint8_t bottomRightValue) {
        const double value = topLeftValue * topLeftWeight + topRightValue * topRightWeight
                           + bottomLeftValue * bottomLeftWeight + bottomRightValue * bottomRightWeight;
        return static_cast<std::uint8_t>(std::clamp(std::lround(value), 0L, 255L));
    };
    return {
        channel(topLeft.red, topRight.red, bottomLeft.red, bottomRight.red),
        channel(topLeft.green, topRight.green, bottomLeft.green, bottomRight.green),
        channel(topLeft.blue, topRight.blue, bottomLeft.blue, bottomRight.blue)
    };
}
