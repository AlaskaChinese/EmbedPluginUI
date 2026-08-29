#pragma once

#include <cstddef>
#include <cstdint>
#include "openoledui/canvas.hpp"

namespace openoledui {

enum class Key : std::uint8_t { Next, Prev, Select, Back };

class Page {
public:
    virtual ~Page() = default;
    virtual void draw(Canvas& canvas, std::uint32_t now_ms) = 0;
    virtual void on_key(Key) {}
};

class Ui {
public:
    static constexpr std::size_t MaxPages = 8;
    bool add_page(Page& page);
    bool remove_page(Page& page);
    void handle(Key key, std::uint32_t now_ms);
    void render(Canvas& canvas, std::uint32_t now_ms);
    std::size_t page_index() const { return current_; }
    std::size_t page_count() const { return count_; }
    bool animating() const { return transition_active_; }

private:
    static float ease_out_cubic(float t);
    void begin_transition(int direction, std::uint32_t now_ms);
    Page* pages_[MaxPages]{};
    std::size_t count_{0};
    std::size_t current_{0};
    std::size_t target_{0};
    int direction_{0};
    std::uint32_t transition_start_{0};
    bool transition_active_{false};
    static constexpr std::uint32_t TransitionMs = 220;
};

} // namespace openoledui
