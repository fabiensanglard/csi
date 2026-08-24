#pragma once

#include "terminal.h"

class TerminalRGB final : public Terminal {
public:
    TerminalRGB();
    void setTextColor(const Rgb& color) override;
    void setBGColor(const Rgb& color) override;
};
