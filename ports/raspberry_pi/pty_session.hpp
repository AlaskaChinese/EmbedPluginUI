#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <sys/types.h>
#include "epui/service_plugin.hpp"
#include "epui/terminal_view.hpp"

namespace epui::rpi {

class PtySessionPlugin final : public epui::ServicePlugin {
public:
    using View = epui::TerminalView<64, 21>;

    explicit PtySessionPlugin(std::string shell = "/bin/sh")
        : shell_(std::move(shell)) {}
    ~PtySessionPlugin() override { stop(); }

    const char* name() const override { return "shell-session"; }
    bool start() override;
    void tick(std::uint32_t) override;
    void stop() override;

    bool send(char ch) { return send(&ch, 1); }
    bool send(const char* data, std::size_t size);
    bool running() const { return child_ > 0 && master_fd_ >= 0; }
    View& view() { return view_; }
    const View& view() const { return view_; }

private:
    void close_master();
    void reap_child();

    std::string shell_;
    View view_;
    int master_fd_{-1};
    pid_t child_{-1};
};

} // namespace epui::rpi
