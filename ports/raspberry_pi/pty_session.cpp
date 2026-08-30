#define _XOPEN_SOURCE 600
#include "pty_session.hpp"
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

namespace epui::rpi {

bool PtySessionPlugin::start() {
    if (running()) return true;
    view_.clear();

    master_fd_ = ::posix_openpt(O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (master_fd_ < 0 || ::grantpt(master_fd_) != 0 || ::unlockpt(master_fd_) != 0) {
        close_master();
        return false;
    }
    char* slave_path = ::ptsname(master_fd_);
    if (!slave_path) {
        close_master();
        return false;
    }
    const std::string slave(slave_path);

    winsize size{};
    size.ws_row = 7;
    size.ws_col = 21;
    ::ioctl(master_fd_, TIOCSWINSZ, &size);
    termios terminal{};
    if (::tcgetattr(master_fd_, &terminal) == 0) {
        terminal.c_lflag &= static_cast<tcflag_t>(~(ECHO | ECHONL));
        ::tcsetattr(master_fd_, TCSANOW, &terminal);
    }

    child_ = ::fork();
    if (child_ < 0) {
        close_master();
        return false;
    }
    if (child_ == 0) {
        ::close(master_fd_);
        if (::setsid() < 0) ::_exit(126);
        const int slave_fd = ::open(slave.c_str(), O_RDWR);
        if (slave_fd < 0) ::_exit(126);
        ::ioctl(slave_fd, TIOCSCTTY, 0);
        ::dup2(slave_fd, STDIN_FILENO);
        ::dup2(slave_fd, STDOUT_FILENO);
        ::dup2(slave_fd, STDERR_FILENO);
        if (slave_fd > STDERR_FILENO) ::close(slave_fd);
        ::setenv("TERM", "dumb", 1);
        ::setenv("PS1", "", 1);
        ::setenv("PS2", "", 1);
        ::setenv("PROMPT", "", 1);
        ::setenv("PROMPT_COMMAND", "", 1);
        ::setenv("ENV", "/dev/null", 1);
        ::setenv("BASH_ENV", "/dev/null", 1);

        const char* name = std::strrchr(shell_.c_str(), '/');
        name = name ? name + 1 : shell_.c_str();
        if (std::strcmp(name, "bash") == 0) {
            ::execl(shell_.c_str(), shell_.c_str(), "--noprofile", "--norc", "-i",
                    static_cast<char*>(nullptr));
        } else if (std::strcmp(name, "zsh") == 0) {
            ::execl(shell_.c_str(), shell_.c_str(), "-f", "-i",
                    static_cast<char*>(nullptr));
        } else {
            ::execl(shell_.c_str(), shell_.c_str(), "-i", static_cast<char*>(nullptr));
        }
        ::_exit(127);
    }
    return true;
}

void PtySessionPlugin::tick(std::uint32_t) {
    if (master_fd_ >= 0) {
        char buffer[256];
        while (true) {
            const ssize_t count = ::read(master_fd_, buffer, sizeof(buffer));
            if (count > 0) {
                view_.feed(buffer, static_cast<std::size_t>(count));
                continue;
            }
            if (count < 0 && errno == EINTR) continue;
            if (count == 0 || (count < 0 && errno == EIO)) close_master();
            break;
        }
    }
    reap_child();
}

bool PtySessionPlugin::send(const char* data, std::size_t size) {
    if (master_fd_ < 0 || (!data && size != 0)) return false;
    std::size_t sent = 0;
    while (sent < size) {
        const ssize_t count = ::write(master_fd_, data + sent, size - sent);
        if (count > 0) sent += static_cast<std::size_t>(count);
        else if (count < 0 && errno == EINTR) continue;
        else return false;
    }
    return true;
}

void PtySessionPlugin::stop() {
    close_master();
    if (child_ > 0) {
        ::kill(child_, SIGHUP);
        pid_t result = 0;
        do {
            result = ::waitpid(child_, nullptr, WNOHANG);
        } while (result < 0 && errno == EINTR);
        if (result == 0) {
            ::kill(child_, SIGKILL);
            while (::waitpid(child_, nullptr, 0) < 0 && errno == EINTR) {}
        }
        child_ = -1;
    }
}

void PtySessionPlugin::close_master() {
    if (master_fd_ >= 0) ::close(master_fd_);
    master_fd_ = -1;
}

void PtySessionPlugin::reap_child() {
    if (child_ <= 0) return;
    const pid_t result = ::waitpid(child_, nullptr, WNOHANG);
    if (result == child_ || (result < 0 && errno == ECHILD)) {
        child_ = -1;
        close_master();
    }
}

} // namespace epui::rpi
