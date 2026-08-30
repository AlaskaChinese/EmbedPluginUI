#pragma once
#include <string>
#include <utility>
#include "epui/terminal_view.hpp"

namespace epui::rpi {

class TerminalFeed {
public:
    using View = epui::TerminalView<64, 21>;

    explicit TerminalFeed(View& view, std::string path = "/tmp/epui-terminal")
        : view_(view), path_(std::move(path)) {}
    ~TerminalFeed();
    bool open_feed();
    void poll();
    const std::string& path() const { return path_; }

private:
    View& view_;
    std::string path_;
    int fd_{-1};
};

} // namespace epui::rpi
