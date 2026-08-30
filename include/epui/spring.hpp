#pragma once

#include <cstdint>

namespace epui {

struct SpringStyle {
    float spring_stiffness{0.30f};
    float spring_damping{0.55f};
    std::uint16_t max_frame_ms{48};
    float settle_position_px{0.05f};
    float settle_velocity{0.05f};
};

class Spring1D {
public:
    void reset(float position) {
        position_ = position;
        target_ = position;
        velocity_ = 0.0f;
    }

    void set_target(float target) { target_ = target; }

    void step(std::uint32_t elapsed_ms, const SpringStyle& style) {
        if (elapsed_ms > style.max_frame_ms) elapsed_ms = style.max_frame_ms;
        while (elapsed_ms > 0) {
            const std::uint32_t step_ms = elapsed_ms > 8 ? 8 : elapsed_ms;
            const float dt = static_cast<float>(step_ms) / 16.0f;
            velocity_ += (target_ - position_) * style.spring_stiffness * dt;
            float damping = 1.0f - style.spring_damping * dt;
            if (damping < 0.0f) damping = 0.0f;
            velocity_ *= damping;
            position_ += velocity_ * dt;
            elapsed_ms -= step_ms;
        }
        if (settled(style)) {
            position_ = target_;
            velocity_ = 0.0f;
        }
    }

    bool settled(const SpringStyle& style) const {
        return absf(target_ - position_) <= style.settle_position_px
            && absf(velocity_) <= style.settle_velocity;
    }

    float position() const { return position_; }
    float target() const { return target_; }
    float velocity() const { return velocity_; }

private:
    static float absf(float value) { return value < 0.0f ? -value : value; }

    float position_{0.0f};
    float target_{0.0f};
    float velocity_{0.0f};
};

} // namespace epui
