#include "pages.hpp"
#include "shell_completion.hpp"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <unistd.h>

int main() {
    epui::Ui ui;
    epui::rpi::PtySessionPlugin shell("/bin/sh");
    epui::rpi::console::TerminalPage page(ui, shell);

    assert(page.captures_key(epui::Key::Select));
    assert(!page.captures_key(epui::Key::Left));
    page.on_key(epui::Key::Select);
    assert(page.focused());
    assert(page.captures_key(epui::Key::Left));
    assert(page.captures_key(epui::Key::ScrollUp));

    page.on_char('a');
    page.on_char('b');
    page.on_char('c');
    page.on_key(epui::Key::Left);
    epui::TerminalControls custom = page.controls();
    custom.cursor_left = epui::Key::Up;
    custom.cursor_right = epui::Key::Down;
    page.set_controls(custom);
    assert(page.captures_key(epui::Key::Up));
    assert(!page.captures_key(epui::Key::Left));
    page.on_key(epui::Key::Up);
    page.on_key(epui::Key::Down);
    page.on_char('\b');
    page.on_char('x');
    assert(std::strcmp(page.command(), "axc") == 0);
    assert(page.command_length() == 3);
    assert(page.cursor() == 2);

    for (int i = 0; i < 16; ++i) shell.view().feed("line\n", 5);
    page.on_key(epui::Key::ScrollUp);
    assert(shell.view().scroll_offset() == 1);
    page.on_key(epui::Key::ScrollDown);
    assert(shell.view().scroll_offset() == 0);

    page.on_key(epui::Key::Select);
    assert(page.command_length() == 0);
    assert(page.cursor() == 0);
    assert(page.history_count() == 1);

    page.set_controls(epui::TerminalControls{});
    page.on_char('d');
    page.on_key(epui::Key::Up);
    assert(std::strcmp(page.command(), "axc") == 0);
    page.on_key(epui::Key::Down);
    assert(std::strcmp(page.command(), "d") == 0);
    page.on_char('\b');

    for (char ch : std::string("ctes")) page.on_char(ch);
    page.on_char('\t');
    assert(std::strcmp(page.command(), "ctest ") == 0);

    char temporary[] = "/tmp/epui-completion-XXXXXX";
    const char* directory = ::mkdtemp(temporary);
    assert(directory);
    const std::string path = std::string(directory) + "/hello world";
    const int file = ::open(path.c_str(), O_CREAT | O_WRONLY, 0600);
    assert(file >= 0);
    ::close(file);
    const std::string partial = std::string("cat ") + directory + "/hel";
    char completed[512]{};
    std::size_t completed_length = 0;
    std::size_t completed_cursor = 0;
    const bool completion_ok = epui::rpi::console::complete_shell_token(
        partial.c_str(), partial.size(), partial.size(), completed,
        sizeof(completed), completed_length, completed_cursor);
    const std::string expected = std::string("cat ") + directory + "/hello\\ world ";
    ::unlink(path.c_str());
    ::rmdir(directory);
    assert(completion_ok);
    assert(std::string(completed) == expected);
    assert(completed_length == expected.size() && completed_cursor == expected.size());

    page.on_key(epui::Key::Back);
    assert(!page.focused());
    shell.stop();
    return 0;
}
