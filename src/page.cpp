#include "openoledui/page.hpp"
#include <algorithm>

namespace openoledui {

bool Ui::add_page(Page& page) { if (count_ >= MaxPages) return false; pages_[count_++] = &page; return true; }
float Ui::ease_out_cubic(float t) { t = std::max(0.0f, std::min(1.0f, t)); const float u = 1.0f - t; return 1.0f - u*u*u; }
void Ui::begin_transition(int direction, std::uint32_t now_ms) { if (count_ < 2 || transition_active_) return; direction_ = direction > 0 ? 1 : -1; target_ = direction_ > 0 ? (current_ + 1) % count_ : (current_ + count_ - 1) % count_; transition_start_ = now_ms; transition_active_ = true; }
void Ui::handle(Key key, std::uint32_t now_ms) { if (count_ == 0) return; if (key == Key::Next) begin_transition(1, now_ms); else if (key == Key::Prev) begin_transition(-1, now_ms); else pages_[current_]->on_key(key); }
void Ui::render(Canvas& canvas, std::uint32_t now_ms) {
    canvas.clear(); if (count_ == 0) return;
    if (!transition_active_) { canvas.reset_origin(); pages_[current_]->draw(canvas, now_ms); }
    else {
        const std::uint32_t elapsed = now_ms - transition_start_;
        const float t = ease_out_cubic(static_cast<float>(elapsed) / static_cast<float>(TransitionMs));
        const int shift = static_cast<int>(t * Canvas::Width + 0.5f);
        canvas.set_origin(-direction_ * shift, 0); pages_[current_]->draw(canvas, now_ms);
        canvas.set_origin(direction_ * (Canvas::Width - shift), 0); pages_[target_]->draw(canvas, now_ms);
        canvas.reset_origin(); if (elapsed >= TransitionMs) { current_ = target_; transition_active_ = false; }
    }
    canvas.reset_origin(); const int dots_w = static_cast<int>(count_ * 4 - 1); int x = (Canvas::Width - dots_w) / 2;
    const std::size_t active = transition_active_ ? target_ : current_;
    for (std::size_t i=0; i<count_; ++i, x+=4) { if (i == active) canvas.fill_rect(x, 61, 3, 2, true); else canvas.pixel(x+1, 62, true); }
}

} // namespace openoledui
