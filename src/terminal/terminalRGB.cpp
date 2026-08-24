#include "terminalRGB.h"

TerminalRGB::TerminalRGB() : Terminal() {}

void TerminalRGB::setTextColor(const Rgb& color) {
    appendColorSequence(csi::colorRgb(color, csi::Layer::Foreground));
}

void TerminalRGB::setBGColor(const Rgb& color) {
    appendColorSequence(csi::colorRgb(color, csi::Layer::Background));
}
