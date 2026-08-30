#include "pages.hpp"
#include <cassert>
#include <cstring>

int main() {
    epui::Ui ui;
    epui::rpi::PtySessionPlugin shell("/bin/sh");
    epui::rpi::console::TerminalPage page(ui, shell);

    assert(page.captures_key(epui::Key::Select));
    assert(!page.captures_key(epui::Key::Next));
    page.on_key(epui::Key::Select);
    assert(page.focused());
    assert(page.captures_key(epui::Key::ScrollUp));

    page.on_char('a');
    page.on_char('b');
    page.on_char('c');
    page.on_key(epui::Key::Prev);
    page.on_char('\b');
    page.on_char('x');
    assert(std::strcmp(page.command(), "axc") == 0);
    assert(page.command_length() == 3);
    assert(page.cursor() == 2);

    for (int i = 0; i < 16; ++i) shell.view().feed("line\n", 5);
    page.on_key(epui::Key::ScrollUp);
    assert(shell.view().scroll_offset() == 7);
    page.on_key(epui::Key::ScrollDown);
    assert(shell.view().scroll_offset() == 0);

    page.on_key(epui::Key::Select);
    assert(page.command_length() == 0);
    assert(page.cursor() == 0);

    page.on_key(epui::Key::Back);
    assert(!page.focused());
    shell.stop();
    return 0;
}
