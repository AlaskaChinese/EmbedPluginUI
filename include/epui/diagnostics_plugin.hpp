#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include "epui/page.hpp"
#include "epui/plugin.hpp"

namespace epui {

enum class DebugMetricView : std::uint8_t {
    Summary,
    Fps,
    Timing,
    Memory,
    Transfer,
};

struct DebugMemoryStats {
    std::uint64_t used_bytes{0};
    std::uint64_t total_bytes{0};
};

using DebugMemoryProbe = bool (*)(void* user, DebugMemoryStats& out);

struct DiagnosticsStyle {
    std::int16_t x{-1};
    std::int16_t y{54};
    std::uint8_t padding{1};
    bool background{true};
    std::uint16_t sample_window_ms{500};
    const char* label{"FPS"};
    DebugMetricView view{DebugMetricView::Fps};
};

class DiagnosticsPlugin final : public Plugin, public UiOverlay {
public:
    explicit DiagnosticsPlugin(Ui& ui, const char* plugin_name = "diagnostics-debug",
                               DiagnosticsStyle style = DiagnosticsStyle{})
        : ui_(ui), name_(plugin_name), style_(style) {}

    const char* name() const override { return name_; }
    PluginKind kind() const override { return PluginKind::Debug; }

    bool start() override {
        if (attached_) return true;
        reset();
        refresh_memory();
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

        char text[32]{};
        format_text(text, sizeof(text));
        const int text_width = canvas.text_width(text);
        int x = style_.x < 0 ? Canvas::Width - text_width - 1 : static_cast<int>(style_.x);
        int y = static_cast<int>(style_.y);
        x = std::max(0, std::min(x, Canvas::Width - std::min(text_width, Canvas::Width)));
        y = std::max(0, std::min(y, Canvas::Height - 7));

        if (style_.background) {
            const int padding = static_cast<int>(style_.padding);
            canvas.fill_rect(x - padding, y - padding,
                             text_width + padding * 2, 7 + padding * 2, false);
        }
        canvas.text(x, y, text);
    }

    // Frame cadence sampler. When auto_sample is enabled Ui::render() calls
    // this through draw_overlay(). Disable auto_sample when successful physical
    // presents should define FPS instead.
    void mark_frame(std::uint32_t now_ms) {
        if (!window_started_) {
            window_started_ = true;
            window_start_ms_ = now_ms;
            last_frame_ms_ = now_ms;
            frames_in_window_ = 0;
            return;
        }

        last_frame_ms_ = now_ms;
        ++frames_in_window_;
        const std::uint32_t elapsed = now_ms - window_start_ms_;
        const std::uint32_t window = style_.sample_window_ms == 0 ? 1u : style_.sample_window_ms;
        if (elapsed < window) return;

        const std::uint32_t scaled = frames_in_window_ * 10000u;
        fps_x10_ = static_cast<std::uint16_t>((scaled + elapsed / 2u) / elapsed);
        if (frames_in_window_ != 0) {
            frame_ms_x10_ = static_cast<std::uint16_t>(
                (elapsed * 10u + frames_in_window_ / 2u) / frames_in_window_);
        }
        fps_valid_ = true;
        frame_time_valid_ = frames_in_window_ != 0;
        refresh_memory();
        window_start_ms_ = now_ms;
        frames_in_window_ = 0;
    }

    // Platform/application instrumentation hooks. Values are intentionally
    // supplied from outside the core so MCU, Linux and simulator backends can
    // use the timer/memory API that is native to that platform.
    void record_render_time_us(std::uint32_t elapsed_us) {
        render_time_us_ = elapsed_us;
        render_time_valid_ = true;
    }

    void record_transfer(std::uint32_t elapsed_us, std::size_t bytes, bool success = true) {
        transfer_time_us_ = elapsed_us;
        transfer_bytes_ = static_cast<std::uint64_t>(bytes);
        transfer_rate_bps_ = elapsed_us == 0
            ? 0u
            : (transfer_bytes_ * 1000000ull) / static_cast<std::uint64_t>(elapsed_us);
        ++transfer_count_;
        if (!success) ++transfer_failures_;
        last_transfer_ok_ = success;
        transfer_valid_ = true;
    }

    void set_memory_bytes(std::uint64_t used_bytes, std::uint64_t total_bytes = 0) {
        memory_.used_bytes = used_bytes;
        memory_.total_bytes = total_bytes;
        memory_valid_ = true;
    }

    void set_memory_probe(DebugMemoryProbe probe, void* user = nullptr) {
        memory_probe_ = probe;
        memory_probe_user_ = user;
        refresh_memory();
    }

    bool refresh_memory() {
        if (!memory_probe_) return memory_valid_;
        DebugMemoryStats next{};
        if (!memory_probe_(memory_probe_user_, next)) return false;
        memory_ = next;
        memory_valid_ = true;
        return true;
    }

    void reset() {
        window_started_ = false;
        fps_valid_ = false;
        frame_time_valid_ = false;
        render_time_valid_ = false;
        transfer_valid_ = false;
        memory_valid_ = false;
        window_start_ms_ = 0;
        last_frame_ms_ = 0;
        frames_in_window_ = 0;
        fps_x10_ = 0;
        frame_ms_x10_ = 0;
        render_time_us_ = 0;
        transfer_time_us_ = 0;
        transfer_bytes_ = 0;
        transfer_rate_bps_ = 0;
        transfer_count_ = 0;
        transfer_failures_ = 0;
        last_transfer_ok_ = true;
        memory_ = {};
    }

    bool attached() const { return attached_; }
    bool visible() const { return visible_; }
    void set_visible(bool visible) { visible_ = visible; }
    bool auto_sample() const { return auto_sample_; }
    void set_auto_sample(bool enabled) { auto_sample_ = enabled; }

    DebugMetricView view() const { return style_.view; }
    void set_view(DebugMetricView view) { style_.view = view; }

    bool fps_valid() const { return fps_valid_; }
    std::uint16_t fps_x10() const { return fps_x10_; }
    float fps() const { return static_cast<float>(fps_x10_) / 10.0f; }

    bool frame_time_valid() const { return frame_time_valid_; }
    std::uint16_t frame_ms_x10() const { return frame_ms_x10_; }
    float frame_time_ms() const { return static_cast<float>(frame_ms_x10_) / 10.0f; }

    bool render_time_valid() const { return render_time_valid_; }
    std::uint32_t render_time_us() const { return render_time_us_; }
    float render_time_ms() const { return static_cast<float>(render_time_us_) / 1000.0f; }

    bool memory_valid() const { return memory_valid_; }
    std::uint64_t memory_used_bytes() const { return memory_.used_bytes; }
    std::uint64_t memory_total_bytes() const { return memory_.total_bytes; }

    bool transfer_valid() const { return transfer_valid_; }
    std::uint32_t transfer_time_us() const { return transfer_time_us_; }
    float transfer_time_ms() const { return static_cast<float>(transfer_time_us_) / 1000.0f; }
    std::uint64_t transfer_bytes() const { return transfer_bytes_; }
    std::uint64_t transfer_rate_bps() const { return transfer_rate_bps_; }
    std::uint32_t transfer_count() const { return transfer_count_; }
    std::uint32_t transfer_failures() const { return transfer_failures_; }
    bool last_transfer_ok() const { return last_transfer_ok_; }

    const DiagnosticsStyle& style() const { return style_; }
    void set_style(const DiagnosticsStyle& style) { style_ = style; }

private:
    static void format_ms(char* out, std::size_t size, std::uint32_t us, bool valid) {
        if (!valid) {
            std::snprintf(out, size, "--");
            return;
        }
        const unsigned whole = static_cast<unsigned>(us / 1000u);
        const unsigned decimal = static_cast<unsigned>((us % 1000u) / 100u);
        std::snprintf(out, size, "%u.%u", whole, decimal);
    }

    static void format_bytes(char* out, std::size_t size, std::uint64_t bytes) {
        if (bytes >= 1024ull * 1024ull) {
            const std::uint64_t x10 = (bytes * 10ull) / (1024ull * 1024ull);
            std::snprintf(out, size, "%llu.%lluM",
                          static_cast<unsigned long long>(x10 / 10ull),
                          static_cast<unsigned long long>(x10 % 10ull));
        } else if (bytes >= 1024ull) {
            const std::uint64_t x10 = (bytes * 10ull) / 1024ull;
            std::snprintf(out, size, "%llu.%lluK",
                          static_cast<unsigned long long>(x10 / 10ull),
                          static_cast<unsigned long long>(x10 % 10ull));
        } else {
            std::snprintf(out, size, "%lluB", static_cast<unsigned long long>(bytes));
        }
    }

    void format_text(char* out, std::size_t size) const {
        switch (style_.view) {
            case DebugMetricView::Summary: {
                if (!fps_valid_ || !frame_time_valid_) {
                    std::snprintf(out, size, "F-- --.-ms");
                    return;
                }
                std::snprintf(out, size, "F%u.%u %u.%ums",
                              static_cast<unsigned>(fps_x10_ / 10u),
                              static_cast<unsigned>(fps_x10_ % 10u),
                              static_cast<unsigned>(frame_ms_x10_ / 10u),
                              static_cast<unsigned>(frame_ms_x10_ % 10u));
                return;
            }
            case DebugMetricView::Timing: {
                char render[10]{};
                char transfer[10]{};
                format_ms(render, sizeof(render), render_time_us_, render_time_valid_);
                format_ms(transfer, sizeof(transfer), transfer_time_us_, transfer_valid_);
                std::snprintf(out, size, "R%s T%sms", render, transfer);
                return;
            }
            case DebugMetricView::Memory: {
                if (!memory_valid_) {
                    std::snprintf(out, size, "MEM --");
                    return;
                }
                char used[12]{};
                char total[12]{};
                format_bytes(used, sizeof(used), memory_.used_bytes);
                if (memory_.total_bytes == 0) {
                    std::snprintf(out, size, "MEM %s", used);
                } else {
                    format_bytes(total, sizeof(total), memory_.total_bytes);
                    std::snprintf(out, size, "MEM %s/%s", used, total);
                }
                return;
            }
            case DebugMetricView::Transfer: {
                if (!transfer_valid_) {
                    std::snprintf(out, size, "TX --");
                    return;
                }
                char time[10]{};
                char bytes[12]{};
                format_ms(time, sizeof(time), transfer_time_us_, true);
                format_bytes(bytes, sizeof(bytes), transfer_bytes_);
                std::snprintf(out, size, "%s%sms %s",
                              last_transfer_ok_ ? "TX " : "TX!", time, bytes);
                return;
            }
            case DebugMetricView::Fps:
            default: {
                const char* label = style_.label ? style_.label : "FPS";
                if (fps_valid_) {
                    std::snprintf(out, size, "%s %u.%u", label,
                                  static_cast<unsigned>(fps_x10_ / 10u),
                                  static_cast<unsigned>(fps_x10_ % 10u));
                } else {
                    std::snprintf(out, size, "%s --", label);
                }
                return;
            }
        }
    }

    Ui& ui_;
    const char* name_;
    DiagnosticsStyle style_{};
    DebugMemoryProbe memory_probe_{nullptr};
    void* memory_probe_user_{nullptr};
    DebugMemoryStats memory_{};

    std::uint32_t window_start_ms_{0};
    std::uint32_t last_frame_ms_{0};
    std::uint32_t frames_in_window_{0};
    std::uint16_t fps_x10_{0};
    std::uint16_t frame_ms_x10_{0};
    std::uint32_t render_time_us_{0};
    std::uint32_t transfer_time_us_{0};
    std::uint64_t transfer_bytes_{0};
    std::uint64_t transfer_rate_bps_{0};
    std::uint32_t transfer_count_{0};
    std::uint32_t transfer_failures_{0};

    bool attached_{false};
    bool visible_{true};
    bool auto_sample_{true};
    bool window_started_{false};
    bool fps_valid_{false};
    bool frame_time_valid_{false};
    bool render_time_valid_{false};
    bool transfer_valid_{false};
    bool memory_valid_{false};
    bool last_transfer_ok_{true};
};

} // namespace epui
