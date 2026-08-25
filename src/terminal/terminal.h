#pragma once

#include "../include/color.h"
#include "../os/terminal_os.h"
#include "csi.h"

#include <cstddef>
#include <memory>
#include <string>

enum class ColorMode {
    Color16,
    Color256,
    Rgb
};

// Which palette entries a nearest match may choose from. The system colors are
// whatever the terminal theme paints them, so matching against them means matching
// against a guess; the cube and the grey ramp have spec-defined levels.
enum class ColorMapping {
    All,    // every entry, 0-255
    Known,  // 16-255: skips the themed system colors, keeps cube and grey ramp
    Cube    // 16-231: the color cube alone, no grey ramp
};

class Terminal {
public:
    Terminal();
    virtual ~Terminal();

    Terminal(const Terminal&) = delete;
    Terminal& operator=(const Terminal&) = delete;

    // Asks the terminal to resize its window to columns x rows and flushes the
    // request, since it has to land before anything is drawn against the new size.
    // Terminals that refuse window manipulation keep their current size, and the
    // display caps how large the window can actually get, so size() stays the
    // authority on what is really there.
    void resize(int columns, int rows);

    os::TerminalSize size() const;

    // The area a full-screen render may paint: two rows short of the real height.
    // endFrame closes the frame with a newline, and the cursor then rests on the
    // line below that. Painting into either row scrolls the terminal and carries the
    // top of the drawing off screen.
    os::TerminalSize drawingArea() const;

    // Writes are buffered; nothing reaches the terminal until commit().
    // beginFrame/endFrame bracket a full-screen render: begin clears the screen and
    // hides the cursor, end restores the cursor, resets attributes, and commits.
    // Output that is not a frame, such as help text, buffers and commits directly.
    void beginFrame();
    // Starts an animation frame that repaints every cell: homes the cursor without
    // clearing, so the terminal never shows a blank screen between frames.
    void beginRepaint();
    void endFrame();
    // Returns the number of bytes flushed, which is what a throughput
    // measurement needs and every other caller is free to ignore.
    std::size_t commit();
    void writeText(const std::string& text);
    void setBold(bool enabled);
    void resetColor();
    // Chooses which palette subset a nearest match may draw from. Returns false when
    // this terminal has no palette to subset, so a caller that was asked for a
    // specific mode can report that it does not apply here.
    virtual bool setColorMapping(ColorMapping mapping);
    virtual void setTextColor(const Rgb& color) = 0;
    virtual void setBGColor(const Rgb& color) = 0;

protected:
    void appendColorSequence(const std::string& sequence);

private:
    std::string buffer_;
    // The color sequence currently in effect, so a run of same-colored cells costs
    // one sequence instead of one per cell. Cleared whenever attributes are reset.
    std::string activeColorSequence_;
};

ColorMode parseColorMode(const std::string& value);
ColorMapping parseColorMapping(const std::string& value);
std::unique_ptr<Terminal> createTerminal(ColorMode colorMode);
