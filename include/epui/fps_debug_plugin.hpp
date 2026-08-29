#pragma once

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include "epui/page.hpp"
#include "epui/plugin.hpp"

namespace epui {

struct FpsDebugStyle {
    std::int16_t x{-1};
    std::int16_t y{54};
    std::uint8_t padding{1};
    bool background{true};
    std::uint16_t sample_window_ms{500};
    const char* label{"FPS"};
};

class FpsDebugPlugin final : public Plugin, public UiOverlay {
public:
    explicit FpsDebugPlugin(Ui& ui, const char* plugin_name = "fps-debug",
                            FpsDebugStyle style = FpsDebugStyle{})
        : ui_(ui), name_(plugin_name), style_(style) {}

    const char* name() const override { return name_; }
    PluginKind kind() const override { return PluginKind::Debug; }

    bool start() override {
        if (attached_) return true;
        reset();
        if (!ui_.add_overlay(*this)) return false;
        attached_ = true;
        return true;
    }

    void stop() override {
        if (!attached_) return;
        ui_.remove_overlay(*this);
        attached_ = false;
    }

    void draw_overlay(Canvas& canvas, std::uint32_t now_ms) override {
        if (auto_sample_) mark_frame(now_ms);
        if (!visible_) return;

        char text[20]{};
        const char* label = style_.label ? style_.label : "FPS";
        if (fps_valid_) {
            const unsigned whole = static_cast<unsigned>(fps_x10_ / 10u);
            const unsigned decimal = static_cast<unsigned>(fps_x10_ % 10u);
            std::snprintf(text, sizeof(text), "%s %u.%u", label, whole, decimal);
        } else {
            std::snprintf(text, sizeof(text), "%s --", label);
        }

        const int text_width = canvas.text_width(text);
        int x = style_.x < 0 ? Canvas::Width - text_width - 1 : static_cast<int>(style_.x);
        int y = static_cast<int>(style_.y);
        x = std::max(0, std::min(x, Canvas::Width - text_width));
        y = std::max(0, std::min(y, Canvas::Height - 7));

        if (style_.background) {
            const int padding = static_cast<int>(style_.padding);
            canvas.fill_rect(x - padding, y - padding,
                             text_width + padding * 2, 7 + padding * 2, false);
        }
        canvas.text(x, y, text);
    }

    void mark_frame(std::uint32_t now_ms) {
        if (!window_started_) {
            window_started_ = true;
            window_start_ms_ = now_ms;
            frames_in_window_ = 0;
            return;
        }

        ++frames_in_window_;
        const std::uint32_t elapsed = now_ms - window_start_ms_;
        const std::uint32_t window = style_.sample_window_ms == 0 ? 1u : style_.sample_window_ms;
        if (elapsed < window) return;

        const std::uint32_t scaled = frames_in_window_ * 10000u;
        fps_x10_ = static_cast<std::uint16_t>((scaled + elapsed / 2u) / elapsed);
        fps_valid_ = true;
        window_start_ms_ = now_ms;
        frames_in_window_ = 0;
    }

    void reset() {
        window_started_ = false;
        fps_valid_ = false;
        window_start_ms_ = 0;
        frames_in_window_ = 0;
        fps_x10_ = 0;
    }

    bool attached() const { return attached_; }
    bool visible() const { return visible_; }
    void set_visible(bool visible) { visible_ = visible; }
    bool auto_sample() const { return auto_sample_; }
    void set_auto_sample(bool enabled) { auto_sample_ = enabled; }
    bool fps_valid() const { return fps_valid_; }
    std::uint16_t fps_x10() const { return fps_x10_; }
    float fps() const { return static_cast<float>(fps_x10_) / 10.0f; }
    const FpsDebugStyle& style() const { return style_; }
    void set_style(const FpsDebugStyle& style) { style_ = style; }

private:
    Ui& ui_;
    const char* name_;
    FpsDebugStyle style_{};
    std::uint32_t window_start_ms_{0};
    std::uint32_t frames_in_window_{0};
    std::uint16_t fps_x10_{0};
    bool attached_{false};
    bool visible_{true};
    bool auto_sample_{true};
    bool window_started_{false};
    bool fps_valid_{false};
};

} // namespace epui
