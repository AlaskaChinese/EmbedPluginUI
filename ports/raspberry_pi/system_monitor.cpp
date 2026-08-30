#include "system_monitor.hpp"
#include <arpa/inet.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <ifaddrs.h>
#include <net/if.h>
#include <pwd.h>
#include <sstream>
#include <sys/statvfs.h>
#include <unistd.h>

namespace epui::rpi {
namespace {

std::uint64_t steady_ms() {
    using namespace std::chrono;
    return static_cast<std::uint64_t>(
        duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

std::string run_command(const char* command) {
    std::string output;
    if (FILE* process = ::popen(command, "r")) {
        char buffer[256];
        while (std::fgets(buffer, sizeof(buffer), process)) output += buffer;
        ::pclose(process);
    }
    return output;
}

} // namespace

void SystemMonitor::sample(StatusSnapshot& snapshot, StatusSection section) {
    switch (section) {
    case StatusSection::Overview: sample_overview(snapshot); break;
    case StatusSection::Network: sample_network(snapshot); break;
    case StatusSection::Power: sample_power(snapshot); break;
    case StatusSection::System: sample_system(snapshot); break;
    case StatusSection::Inactive: break;
    }
}

void SystemMonitor::sample_overview(StatusSnapshot& snapshot) {
    snapshot.temperature_c = read_float_file(
        "/sys/class/thermal/thermal_zone0/temp", 0.001f);

    std::uint64_t total = 0;
    std::uint64_t idle = 0;
    if (read_cpu(total, idle)) {
        if (prev_cpu_total_ != 0 && total > prev_cpu_total_) {
            const auto total_delta = total - prev_cpu_total_;
            const auto idle_delta = idle - prev_cpu_idle_;
            snapshot.cpu_percent = 100.0f
                * static_cast<float>(total_delta - idle_delta)
                / static_cast<float>(total_delta);
        }
        prev_cpu_total_ = total;
        prev_cpu_idle_ = idle;
    }

    std::ifstream memory("/proc/meminfo");
    std::string key;
    std::string unit;
    std::uint64_t value = 0;
    std::uint64_t total_kib = 0;
    std::uint64_t available_kib = 0;
    while (memory >> key >> value >> unit) {
        if (key == "MemTotal:") total_kib = value;
        else if (key == "MemAvailable:") available_kib = value;
    }
    if (total_kib != 0) {
        snapshot.memory_percent = 100.0f
            * static_cast<float>(total_kib - available_kib)
            / static_cast<float>(total_kib);
    }

    double loads[1]{};
    if (::getloadavg(loads, 1) == 1) snapshot.load1 = loads[0];
}

void SystemMonitor::sample_network(StatusSnapshot& snapshot) {
    snapshot.interface = "?";
    snapshot.ipv4 = "0.0.0.0";

    ifaddrs* addresses = nullptr;
    if (::getifaddrs(&addresses) == 0) {
        for (ifaddrs* address = addresses; address; address = address->ifa_next) {
            if (!address->ifa_addr || address->ifa_addr->sa_family != AF_INET
                || (address->ifa_flags & IFF_LOOPBACK) || !(address->ifa_flags & IFF_UP)) {
                continue;
            }
            char ip[INET_ADDRSTRLEN]{};
            auto* socket_address = reinterpret_cast<sockaddr_in*>(address->ifa_addr);
            if (::inet_ntop(AF_INET, &socket_address->sin_addr, ip, sizeof(ip))) {
                snapshot.interface = address->ifa_name;
                snapshot.ipv4 = ip;
                break;
            }
        }
        ::freeifaddrs(addresses);
    }

    std::uint64_t rx = 0;
    std::uint64_t tx = 0;
    const auto now = steady_ms();
    if (!read_network(snapshot.interface, rx, tx)) {
        snapshot.rx_kib_s = 0;
        snapshot.tx_kib_s = 0;
        return;
    }
    if (prev_net_ms_ != 0 && prev_iface_ == snapshot.interface && now > prev_net_ms_
        && rx >= prev_rx_ && tx >= prev_tx_) {
        const float seconds = static_cast<float>(now - prev_net_ms_) / 1000.0f;
        snapshot.rx_kib_s = static_cast<float>(rx - prev_rx_) / 1024.0f / seconds;
        snapshot.tx_kib_s = static_cast<float>(tx - prev_tx_) / 1024.0f / seconds;
    } else {
        snapshot.rx_kib_s = 0;
        snapshot.tx_kib_s = 0;
    }
    prev_rx_ = rx;
    prev_tx_ = tx;
    prev_net_ms_ = now;
    prev_iface_ = snapshot.interface;
}

void SystemMonitor::sample_power(StatusSnapshot& snapshot) {
    if (::access("/usr/bin/vcgencmd", X_OK) != 0) {
        snapshot.supply_voltage_v = -1;
        snapshot.core_current_a = -1;
        snapshot.throttled = 0;
        return;
    }
    snapshot.supply_voltage_v = vcgencmd_value(
        "vcgencmd pmic_read_adc EXT5V_V 2>/dev/null");
    snapshot.core_current_a = vcgencmd_value(
        "vcgencmd pmic_read_adc VDD_CORE_A 2>/dev/null");
    snapshot.throttled = vcgencmd_throttled();
}

void SystemMonitor::sample_system(StatusSnapshot& snapshot) {
    struct statvfs disk {};
    if (::statvfs("/", &disk) == 0 && disk.f_blocks != 0) {
        snapshot.disk_percent = 100.0f
            * static_cast<float>(disk.f_blocks - disk.f_bavail)
            / static_cast<float>(disk.f_blocks);
    }
    char hostname[128]{};
    if (::gethostname(hostname, sizeof(hostname) - 1) == 0) snapshot.hostname = hostname;
    if (passwd* user = ::getpwuid(::geteuid())) snapshot.user = user->pw_name;
    const float uptime = read_float_file("/proc/uptime");
    snapshot.uptime_s = uptime < 0 ? 0 : static_cast<std::uint64_t>(uptime);
}

float SystemMonitor::read_float_file(const char* path, float scale) {
    std::ifstream file(path);
    float value = -1;
    return file >> value ? value * scale : -1;
}

float SystemMonitor::vcgencmd_value(const char* command) {
    const std::string output = run_command(command);
    const auto equals = output.find('=');
    if (equals == std::string::npos) return -1;
    char* end = nullptr;
    const float value = std::strtof(output.c_str() + equals + 1, &end);
    return end == output.c_str() + equals + 1 ? -1 : value;
}

std::uint32_t SystemMonitor::vcgencmd_throttled() {
    const std::string output = run_command("vcgencmd get_throttled 2>/dev/null");
    const auto hex = output.find("0x");
    return hex == std::string::npos
        ? 0 : static_cast<std::uint32_t>(std::strtoul(output.c_str() + hex, nullptr, 16));
}

bool SystemMonitor::read_cpu(std::uint64_t& total, std::uint64_t& idle) const {
    std::ifstream file("/proc/stat");
    std::string cpu;
    std::uint64_t user = 0;
    std::uint64_t nice = 0;
    std::uint64_t system = 0;
    std::uint64_t idle_time = 0;
    std::uint64_t io_wait = 0;
    std::uint64_t irq = 0;
    std::uint64_t soft_irq = 0;
    std::uint64_t steal = 0;
    if (!(file >> cpu >> user >> nice >> system >> idle_time >> io_wait
          >> irq >> soft_irq >> steal) || cpu != "cpu") {
        return false;
    }
    idle = idle_time + io_wait;
    total = user + nice + system + idle_time + io_wait + irq + soft_irq + steal;
    return true;
}

bool SystemMonitor::read_network(const std::string& interface,
                                 std::uint64_t& rx, std::uint64_t& tx) const {
    std::ifstream file("/proc/net/dev");
    std::string line;
    while (std::getline(file, line)) {
        const auto colon = line.find(':');
        if (colon == std::string::npos) continue;
        std::string name = line.substr(0, colon);
        const auto first = name.find_first_not_of(" \t");
        const auto last = name.find_last_not_of(" \t");
        if (first == std::string::npos || last == std::string::npos) continue;
        name = name.substr(first, last - first + 1);
        if (name != interface) continue;

        std::istringstream values(line.substr(colon + 1));
        std::uint64_t ignored[14]{};
        return static_cast<bool>(values >> rx >> ignored[0] >> ignored[1] >> ignored[2]
            >> ignored[3] >> ignored[4] >> ignored[5] >> ignored[6] >> tx
            >> ignored[7] >> ignored[8] >> ignored[9] >> ignored[10]
            >> ignored[11] >> ignored[12] >> ignored[13]);
    }
    return false;
}

} // namespace epui::rpi
