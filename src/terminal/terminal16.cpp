#include "terminal16.h"

#include <climits>

namespace {

// Index of the palette entry closest to the requested color, by squared distance
// in gamma-encoded sRGB.
int quantize(const Rgb& color) {
    int best = 0;
    int bestDistance = INT_MAX;
    for (int index = 0; index < csi::paletteSize; ++index) {
        const csi::PaletteEntry& candidate = csi::palette[index];
        const int red = static_cast<int>(color.red) - candidate.red;
        const int green = static_cast<int>(color.green) - candidate.green;
        const int blue = static_cast<int>(color.blue) - candidate.blue;
        const int distance = red * red + green * green + blue * blue;
        if (distance < bestDistance) {
            best = index;
            bestDistance = distance;
        }
    }
    return best;
}

}  // namespace

Terminal16::Terminal16() : Terminal() {}

void Terminal16::setTextColor(const Rgb& color) {
    appendColorSequence(csi::color16(quantize(color), csi::Layer::Foreground));
}

void Terminal16::setBGColor(const Rgb& color) {
    appendColorSequence(csi::color16(quantize(color), csi::Layer::Background));
}
