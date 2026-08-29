#pragma once

#include "epui/plugin.hpp"

namespace epui {

class ServicePlugin : public Plugin {
public:
    PluginKind kind() const final { return PluginKind::Service; }
};

class PeriodicServicePlugin : public ServicePlugin {
public:
    explicit PeriodicServicePlugin(std::uint32_t interval_ms) : interval_ms_(interval_ms) {}

    void tick(std::uint32_t now_ms) final {
        if (!has_run_ || interval_ms_ == 0 || static_cast<std::uint32_t>(now_ms - last_run_ms_) >= interval_ms_) {
            last_run_ms_ = now_ms;
            has_run_ = true;
            update(now_ms);
        }
    }

    std::uint32_t interval_ms() const { return interval_ms_; }
    void set_interval_ms(std::uint32_t interval_ms) { interval_ms_ = interval_ms; }

protected:
    virtual void update(std::uint32_t now_ms) = 0;
    void reset_schedule() { has_run_ = false; last_run_ms_ = 0; }

private:
    std::uint32_t interval_ms_;
    std::uint32_t last_run_ms_{0};
    bool has_run_{false};
};

} // namespace epui
