#pragma once

#include <cstdint>
#include "epui/page.hpp"

namespace epui::demo {

class SensorPage final : public Page {
public:
    void draw(Canvas& canvas, std::uint32_t now_ms) override;
};

} // namespace epui::demo
