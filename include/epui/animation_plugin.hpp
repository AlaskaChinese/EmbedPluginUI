#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include "epui/plugin.hpp"

namespace epui {

enum class Easing : std::uint8_t {
    Linear,
    EaseOutCubic,
    EaseInOutCubic,
};

template <std::size_t MaxTracks = 8>
class AnimationPlugin final : public Plugin {
public:
    explicit AnimationPlugin(const char* plugin_name = "animation") : name_(plugin_name) {}

    const char* name() const override { return name_; }
    PluginKind kind() const override { return PluginKind::Animation; }

    bool animate(std::size_t track, float from, float to, std::uint32_t duration_ms,
                 std::uint32_t now_ms, Easing easing = Easing::EaseOutCubic) {
        if (track >= MaxTracks) return false;
        Track& t = tracks_[track];
        t.from = from;
        t.to = to;
        t.value = from;
        t.started_ms = now_ms;
        t.duration_ms = duration_ms;
        t.easing = easing;
        t.active = duration_ms != 0;
        if (!t.active) t.value = to;
        return true;
    }

    void tick(std::uint32_t now_ms) override {
        for (auto& t : tracks_) {
            if (!t.active) continue;
            const std::uint32_t elapsed = now_ms - t.started_ms;
            if (elapsed >= t.duration_ms) {
                t.value = t.to;
                t.active = false;
                continue;
            }
            const float normalized = static_cast<float>(elapsed) / static_cast<float>(t.duration_ms);
            const float eased = apply_easing(normalized, t.easing);
            t.value = t.from + (t.to - t.from) * eased;
        }
    }

    float value(std::size_t track) const { return track < MaxTracks ? tracks_[track].value : 0.0f; }
    bool active(std::size_t track) const { return track < MaxTracks && tracks_[track].active; }
    void cancel(std::size_t track, bool snap_to_end = false) {
        if (track >= MaxTracks) return;
        Track& t = tracks_[track];
        if (snap_to_end) t.value = t.to;
        t.active = false;
    }
    void cancel_all(bool snap_to_end = false) {
        for (std::size_t i = 0; i < MaxTracks; ++i) cancel(i, snap_to_end);
    }

private:
    struct Track {
        float from{0.0f};
        float to{0.0f};
        float value{0.0f};
        std::uint32_t started_ms{0};
        std::uint32_t duration_ms{0};
        Easing easing{Easing::EaseOutCubic};
        bool active{false};
    };

    static float apply_easing(float t, Easing easing) {
        t = std::max(0.0f, std::min(1.0f, t));
        switch (easing) {
            case Easing::Linear:
                return t;
            case Easing::EaseInOutCubic:
                return t < 0.5f ? 4.0f * t * t * t
                                : 1.0f - ((-2.0f * t + 2.0f) * (-2.0f * t + 2.0f) * (-2.0f * t + 2.0f)) / 2.0f;
            case Easing::EaseOutCubic:
            default: {
                const float u = 1.0f - t;
                return 1.0f - u * u * u;
            }
        }
    }

    const char* name_;
    Track tracks_[MaxTracks]{};
};

} // namespace epui
