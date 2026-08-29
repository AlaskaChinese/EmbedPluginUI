#include "epui/page.hpp"
#include <algorithm>

namespace epui {

bool Ui::add_page(Page& page) {
    if (count_ >= MaxPages) return false;
    for (std::size_t i = 0; i < count_; ++i) if (pages_[i] == &page) return false;
    pages_[count_++] = &page;
    return true;
}

bool Ui::remove_page(Page& page) {
    for (std::size_t i = 0; i < count_; ++i) {
        if (pages_[i] != &page) continue;
        for (std::size_t j = i + 1; j < count_; ++j) pages_[j - 1] = pages_[j];
        pages_[--count_] = nullptr;
        transition_active_ = false;
        transition_has_time_ = false;
        transition_position_ = 0.0f;
        transition_velocity_ = 0.0f;
        target_ = current_ = count_ == 0 ? 0 : std::min(current_, count_ - 1);
        return true;
    }
    return false;
}

bool Ui::add_overlay(UiOverlay& overlay) {
    if (overlay_count_ >= MaxOverlays) return false;
    for (std::size_t i = 0; i < overlay_count_; ++i) if (overlays_[i] == &overlay) return false;
    overlays_[overlay_count_++] = &overlay;
    return true;
}

bool Ui::remove_overlay(UiOverlay& overlay) {
    for (std::size_t i = 0; i < overlay_count_; ++i) {
        if (overlays_[i] != &overlay) continue;
        for (std::size_t j = i + 1; j < overlay_count_; ++j) overlays_[j - 1] = overlays_[j];
        overlays_[--overlay_count_] = nullptr;
        return true;
    }
    return false;
}

void Ui::reset_transition_motion(std::uint32_t now_ms) {
    transition_position_ = 0.0f;
    transition_velocity_ = 0.0f;
    transition_last_ms_ = now_ms;
    transition_has_time_ = true;
}

void Ui::begin_transition(int direction, std::uint32_t now_ms) {
    if (count_ < 2 || transition_active_) return;
    direction_ = direction > 0 ? 1 : -1;
    target_ = direction_ > 0 ? (current_ + 1) % count_ : (current_ + count_ - 1) % count_;
    reset_transition_motion(now_ms);
    transition_active_ = true;
}

void Ui::handle(Key key, std::uint32_t now_ms) {
    if (count_ == 0) return;
    Page* page = pages_[current_];
    if (page->captures_key(key)) {
        page->on_key(key);
        return;
    }
    if (key == Key::Next) begin_transition(1, now_ms);
    else if (key == Key::Prev) begin_transition(-1, now_ms);
    else page->on_key(key);
}

void Ui::step_transition_spring(float dt) {
    const float target = static_cast<float>(Canvas::Width);
    transition_velocity_ += (target - transition_position_) * transition_style_.spring_stiffness * dt;
    float damping = 1.0f - transition_style_.spring_damping * dt;
    if (damping < 0.0f) damping = 0.0f;
    transition_velocity_ *= damping;
    transition_position_ += transition_velocity_ * dt;
}

void Ui::advance_transition(std::uint32_t now_ms) {
    if (!transition_has_time_) {
        transition_last_ms_ = now_ms;
        transition_has_time_ = true;
        return;
    }

    std::uint32_t elapsed = now_ms - transition_last_ms_;
    transition_last_ms_ = now_ms;
    if (elapsed > transition_style_.max_frame_ms) elapsed = transition_style_.max_frame_ms;

    while (elapsed > 0) {
        const std::uint32_t step_ms = elapsed > 8 ? 8 : elapsed;
        const float dt = static_cast<float>(step_ms) / 16.0f;
        step_transition_spring(dt);
        elapsed -= step_ms;
    }

    if (transition_settled()) {
        transition_position_ = static_cast<float>(Canvas::Width);
        transition_velocity_ = 0.0f;
    }
}

bool Ui::transition_settled() const {
    const float target = static_cast<float>(Canvas::Width);
    return absf(target - transition_position_) <= transition_style_.settle_position_px
        && absf(transition_velocity_) <= transition_style_.settle_velocity;
}

void Ui::draw_page_dots(Canvas& canvas) const {
    if (count_ == 0) return;
    const int dots_w = static_cast<int>(count_ * 4 - 1);
    int x = (Canvas::Width - dots_w) / 2;
    const std::size_t active = transition_active_ ? target_ : current_;
    for (std::size_t i = 0; i < count_; ++i, x += 4) {
        if (i == active) canvas.fill_rect(x, 61, 3, 2, true);
        else canvas.pixel(x + 1, 62, true);
    }
}

void Ui::draw_overlays(Canvas& canvas, std::uint32_t now_ms) {
    canvas.reset_origin();
    for (std::size_t i = 0; i < overlay_count_; ++i) {
        if (overlays_[i]) overlays_[i]->draw_overlay(canvas, now_ms);
    }
}

void Ui::render(Canvas& canvas, std::uint32_t now_ms) {
    canvas.clear();

    if (count_ != 0) {
        if (!transition_active_) {
            canvas.reset_origin();
            pages_[current_]->draw(canvas, now_ms);
        } else {
            advance_transition(now_ms);
            const int shift = round_to_int(transition_position_);
            canvas.set_origin(-direction_ * shift, 0);
            pages_[current_]->draw(canvas, now_ms);
            canvas.set_origin(direction_ * (Canvas::Width - shift), 0);
            pages_[target_]->draw(canvas, now_ms);
            canvas.reset_origin();

            if (transition_settled()) {
                current_ = target_;
                transition_active_ = false;
                transition_has_time_ = false;
            }
        }

        canvas.reset_origin();
        draw_page_dots(canvas);
    }

    draw_overlays(canvas, now_ms);
    canvas.reset_origin();
}

} // namespace epui
