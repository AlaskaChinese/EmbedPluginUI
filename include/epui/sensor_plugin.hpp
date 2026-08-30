#pragma once

#include <cstddef>
#include "epui/plugin.hpp"

namespace epui {

template <typename Snapshot>
class SensorPlugin : public Plugin {
public:
    explicit SensorPlugin(std::uint32_t interval_ms) : interval_ms_(interval_ms) {}

    PluginKind kind() const final { return PluginKind::Sensor; }

    void tick(std::uint32_t now_ms) final {
        if (sampled_once_ && interval_ms_ != 0 && static_cast<std::uint32_t>(now_ms - last_attempt_ms_) < interval_ms_) return;
        last_attempt_ms_ = now_ms;
        sampled_once_ = true;
        Snapshot next = snapshot_;
        last_sample_ok_ = sample(next, now_ms);
        if (last_sample_ok_) {
            snapshot_ = next;
            valid_ = true;
            ++sample_count_;
        }
    }

    const Snapshot& snapshot() const { return snapshot_; }
    bool valid() const { return valid_; }
    bool last_sample_ok() const { return last_sample_ok_; }
    std::size_t sample_count() const { return sample_count_; }
    std::uint32_t interval_ms() const { return interval_ms_; }
    void set_interval_ms(std::uint32_t interval_ms) { interval_ms_ = interval_ms; }

protected:
    virtual bool sample(Snapshot& out, std::uint32_t now_ms) = 0;
    void reset_schedule() { sampled_once_ = false; }

private:
    Snapshot snapshot_{};
    std::uint32_t interval_ms_{0};
    std::uint32_t last_attempt_ms_{0};
    std::size_t sample_count_{0};
    bool sampled_once_{false};
    bool valid_{false};
    bool last_sample_ok_{false};
};

} // namespace epui
