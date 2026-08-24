#pragma once

#include "terminal.h"

class Terminal16 final : public Terminal {
public:
    Terminal16();
    void setTextColor(const Rgb& color) override;
    void setBGColor(const Rgb& color) override;
};
