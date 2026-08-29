#pragma once
#include <cstdint>
#include <string>
namespace openoledui::rpi {
struct StatusSnapshot{float temperature_c{-1},cpu_percent{-1},memory_percent{-1},disk_percent{-1},supply_voltage_v{-1},core_current_a{-1},rx_kib_s{0},tx_kib_s{0};double load1{0};std::uint64_t uptime_s{0};std::uint32_t throttled{0};std::string user{"?"},hostname{"?"},interface{"?"},ipv4{"0.0.0.0"};};
class SystemMonitor{public:StatusSnapshot sample();private:bool read_cpu(std::uint64_t&,std::uint64_t&)const;bool read_network(const std::string&,std::uint64_t&,std::uint64_t&)const;static float read_float_file(const char*,float scale=1.0f);static float vcgencmd_value(const char*);static std::uint32_t vcgencmd_throttled();std::uint64_t prev_cpu_total_{0},prev_cpu_idle_{0},prev_rx_{0},prev_tx_{0},prev_net_ms_{0};std::string prev_iface_;};
}
