#include "terminal_feed.hpp"
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace epui::rpi {

TerminalFeed::~TerminalFeed() {
    if (fd_ >= 0) ::close(fd_);
}

bool TerminalFeed::open_feed() {
    if (fd_ >= 0) return true;
    if (::mkfifo(path_.c_str(), 0666) < 0 && errno != EEXIST) return false;
    fd_ = ::open(path_.c_str(), O_RDWR | O_NONBLOCK);
    return fd_ >= 0;
}

void TerminalFeed::poll() {
    if (fd_ < 0) return;
    char buffer[256];
    ssize_t count = 0;
    while ((count = ::read(fd_, buffer, sizeof(buffer))) > 0) {
        view_.feed(buffer, static_cast<std::size_t>(count));
    }
}

} // namespace epui::rpi
