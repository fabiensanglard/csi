#include "terminal.h"

#include "terminal16.h"
#include "terminal256.h"
#include "terminalRGB.h"

#include <stdexcept>

Terminal::Terminal() {
    os::prepareTerminal();
}

Terminal::~Terminal() {
    os::restoreTerminal();
}

void Terminal::resize(int columns, int rows) {
    buffer_ += csi::resize(columns, rows);
    commit();
}

os::TerminalSize Terminal::size() const {
    return os::terminalSize();
}

os::TerminalSize Terminal::drawingArea() const {
    // Two rows are spent on the carriage return that ends the frame: one for the
    // return itself, one for the line the cursor then rests on. Drawing into either
    // scrolls the terminal, which pushes the top of the drawing off screen.
    constexpr int rowsReservedForCursor = 2;
    const os::TerminalSize actual = size();
    const int drawableRows = actual.rows > rowsReservedForCursor
                                 ? actual.rows - rowsReservedForCursor
                                 : 1;
    return {actual.columns, drawableRows};
}

void Terminal::beginFrame() {
    buffer_ += csi::eraseDisplay() + csi::cursorHome() + csi::hideCursor();
    activeColorSequence_.clear();
}

void Terminal::beginRepaint() {
    buffer_ += csi::cursorHome() + csi::hideCursor();
    activeColorSequence_.clear();
}

void Terminal::endFrame() {
    buffer_ += csi::reset() + csi::showCursor() + "\n";
    activeColorSequence_.clear();
    commit();
}

void Terminal::commit() {
    if (!buffer_.empty()) {
        os::write(buffer_);
        buffer_.clear();
    }
}

void Terminal::writeText(const std::string& text) {
    buffer_ += text;
}

void Terminal::setBold(bool enabled) {
    buffer_ += csi::bold(enabled);
}

bool Terminal::setColorMapping(ColorMapping) {
    return false;
}

void Terminal::resetColor() {
    buffer_ += csi::reset();
    activeColorSequence_.clear();
}

void Terminal::appendColorSequence(const std::string& sequence) {
    if (sequence == activeColorSequence_) {
        return;
    }
    buffer_ += sequence;
    activeColorSequence_ = sequence;
}

ColorMode parseColorMode(const std::string& value) {
    if (value == "16") {
        return ColorMode::Color16;
    }
    if (value == "256") {
        return ColorMode::Color256;
    }
    if (value == "RGB" || value == "rgb") {
        return ColorMode::Rgb;
    }
    throw std::invalid_argument("--space must be 16, 256, or RGB");
}

ColorMapping parseColorMapping(const std::string& value) {
    if (value == "all") {
        return ColorMapping::All;
    }
    if (value == "known") {
        return ColorMapping::Known;
    }
    if (value == "cube") {
        return ColorMapping::Cube;
    }
    throw std::invalid_argument("--mode must be all, known, or cube");
}

std::unique_ptr<Terminal> createTerminal(ColorMode colorMode) {
    switch (colorMode) {
    case ColorMode::Color16:
        return std::make_unique<Terminal16>();
    case ColorMode::Color256:
        return std::make_unique<Terminal256>();
    case ColorMode::Rgb:
        break;
    }
    return std::make_unique<TerminalRGB>();
}

