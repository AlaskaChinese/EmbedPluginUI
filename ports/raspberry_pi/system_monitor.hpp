#pragma once

#include <cstdint>
#include <string>

namespace epui::rpi {

enum class StatusSection : std::uint8_t { Inactive, Overview, Network, Power, System };

struct StatusSnapshot {
    float temperature_c{-1};
    float cpu_percent{-1};
    float memory_percent{-1};
    float disk_percent{-1};
    float supply_voltage_v{-1};
    float core_current_a{-1};
    float rx_kib_s{0};
    float tx_kib_s{0};
    double load1{0};
    std::uint64_t uptime_s{0};
    std::uint32_t throttled{0};
    std::string user{"?"};
    std::string hostname{"?"};
    std::string interface{"?"};
    std::string ipv4{"0.0.0.0"};
};

class SystemMonitor {
public:
    void sample(StatusSnapshot& snapshot, StatusSection section);

private:
    void sample_overview(StatusSnapshot& snapshot);
    void sample_network(StatusSnapshot& snapshot);
    void sample_power(StatusSnapshot& snapshot);
    static void sample_system(StatusSnapshot& snapshot);
    bool read_cpu(std::uint64_t& total, std::uint64_t& idle) const;
    bool read_network(const std::string& interface, std::uint64_t& rx,
                      std::uint64_t& tx) const;
    static float read_float_file(const char* path, float scale = 1.0f);
    static float vcgencmd_value(const char* command);
    static std::uint32_t vcgencmd_throttled();

    std::uint64_t prev_cpu_total_{0};
    std::uint64_t prev_cpu_idle_{0};
    std::uint64_t prev_rx_{0};
    std::uint64_t prev_tx_{0};
    std::uint64_t prev_net_ms_{0};
    std::string prev_iface_;
};

} // namespace epui::rpi
