#include "pty_session.hpp"
#include <cassert>
#include <chrono>
#include <cstring>
#include <thread>

namespace {

bool contains(const epui::rpi::PtySessionPlugin::View& view, const char* text) {
    for (std::size_t i = 0; i < view.line_count(); ++i) {
        if (std::strstr(view.line(i), text)) return true;
    }
    return false;
}

} // namespace

int main() {
    epui::rpi::PtySessionPlugin shell("/bin/sh");
    assert(shell.start());
    const char command[] = "printf '\\105\\120\\125\\111\\137\\120\\124\\131\\137\\117\\113\\n'\r";
    assert(shell.send(command, sizeof(command) - 1));

    for (int i = 0; i < 200 && !contains(shell.view(), "EPUI_PTY_OK"); ++i) {
        shell.tick(0);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    assert(contains(shell.view(), "EPUI_PTY_OK"));
    assert(!contains(shell.view(), "printf"));
    shell.send("exit\r", 5);
    shell.stop();
    return 0;
}
