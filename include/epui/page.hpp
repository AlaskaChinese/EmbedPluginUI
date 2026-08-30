#pragma once

#include <cstddef>
#include <cstdint>
#include "epui/canvas.hpp"

namespace epui {

enum class Key : std::uint8_t {
    Next,
    Prev,
    Select,
    Back,
    ScrollUp,
    ScrollDown,
};
struct InputEvent;

class Page {
public:
    virtual ~Page() = default;
    virtual void draw(Canvas& canvas, std::uint32_t now_ms) = 0;
    virtual void on_key(Key) {}
    virtual void on_char(char) {}
    virtual bool captures_key(Key) const { return false; }
};

class UiOverlay {
public:
    virtual ~UiOverlay() = default;
    virtual void draw_overlay(Canvas& canvas, std::uint32_t now_ms) = 0;
};

struct PageTransitionStyle {
    float spring_stiffness{0.30f};
    float spring_damping{0.55f};
    std::uint16_t max_frame_ms{48};
    float settle_position_px{0.05f};
    float settle_velocity{0.05f};
};

class Ui {
public:
    static constexpr std::size_t MaxPages = 8;
    static constexpr std::size_t MaxOverlays = 4;

    explicit Ui(const PageTransitionStyle& transition_style = PageTransitionStyle{})
        : transition_style_(transition_style) {}

    bool add_page(Page& page);
    bool remove_page(Page& page);
    bool add_overlay(UiOverlay& overlay);
    bool remove_overlay(UiOverlay& overlay);
    void handle(Key key, std::uint32_t now_ms);
    void handle(const InputEvent& event, std::uint32_t now_ms);
    void render(Canvas& canvas, std::uint32_t now_ms);
    std::size_t page_index() const { return current_; }
    std::size_t page_count() const { return count_; }
    std::size_t overlay_count() const { return overlay_count_; }
    bool animating() const { return transition_active_; }
    float transition_position() const { return transition_position_; }
    float transition_velocity() const { return transition_velocity_; }
    const PageTransitionStyle& transition_style() const { return transition_style_; }
    void set_transition_style(const PageTransitionStyle& style) { transition_style_ = style; }

private:
    static float absf(float value) { return value < 0.0f ? -value : value; }
    static int round_to_int(float value) {
        return static_cast<int>(value >= 0.0f ? value + 0.5f : value - 0.5f);
    }

    void begin_transition(int direction, std::uint32_t now_ms);
    void reset_transition_motion(std::uint32_t now_ms);
    void step_transition_spring(float dt);
    void advance_transition(std::uint32_t now_ms);
    bool transition_settled() const;
    void draw_page_dots(Canvas& canvas) const;
    void draw_overlays(Canvas& canvas, std::uint32_t now_ms);

    Page* pages_[MaxPages]{};
    UiOverlay* overlays_[MaxOverlays]{};
    std::size_t count_{0};
    std::size_t overlay_count_{0};
    std::size_t current_{0};
    std::size_t target_{0};
    int direction_{0};
    float transition_position_{0.0f};
    float transition_velocity_{0.0f};
    std::uint32_t transition_last_ms_{0};
    bool transition_has_time_{false};
    bool transition_active_{false};
    PageTransitionStyle transition_style_{};
};

} // namespace epui
