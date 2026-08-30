#include "epui/terminal_view.hpp"
#include "epui/terminal_controls.hpp"
#include "epui/terminal_line_editor.hpp"
#include <cassert>
#include <cstring>

namespace {

bool pixel_on(const epui::Canvas& canvas, int x, int y) {
    const std::size_t index = static_cast<std::size_t>(x + (y / 8) * epui::Canvas::Width);
    return ((canvas.data()[index] >> (y & 7)) & 1u) != 0u;
}

} // namespace

int main() {
    epui::TerminalControls controls;
    assert(controls.action_for(epui::Key::Select, false) == epui::TerminalAction::Focus);
    assert(controls.action_for(epui::Key::Left, true) == epui::TerminalAction::CursorLeft);
    assert(controls.action_for(epui::Key::Right, true) == epui::TerminalAction::CursorRight);
    assert(controls.action_for(epui::Key::ScrollUp, true) == epui::TerminalAction::OutputUp);
    assert(controls.action_for(epui::Key::Up, true) == epui::TerminalAction::HistoryPrevious);
    controls.cursor_left = epui::Key::Up;
    assert(controls.action_for(epui::Key::Up, true) == epui::TerminalAction::CursorLeft);

    epui::TerminalLineEditor<8, 2> editor;
    assert(editor.insert('a') && editor.insert('b'));
    assert(editor.move_left() && editor.insert('x'));
    assert(std::strcmp(editor.command(), "axb") == 0);
    editor.commit();
    assert(editor.history_count() == 1 && editor.length() == 0);
    editor.insert('d');
    assert(editor.previous_history());
    assert(std::strcmp(editor.command(), "axb") == 0);
    assert(editor.next_history());
    assert(std::strcmp(editor.command(), "d") == 0);
    editor.commit();
    for (char ch : "three") {
        if (ch != 0) editor.insert(ch);
    }
    editor.commit();
    assert(editor.history_count() == 2);
    assert(editor.previous_history());
    assert(std::strcmp(editor.command(), "three") == 0);
    assert(editor.previous_history());
    assert(std::strcmp(editor.command(), "d") == 0);

    epui::TerminalView<8, 8> view;
    assert(view.line_count() == 1);
    assert(std::strcmp(view.line(0), "") == 0);

    view.feed("abc\rZ", 5);
    assert(std::strcmp(view.line(0), "Zbc") == 0);
    assert(view.cursor_column() == 1);
    view.feed('\b');
    view.feed('Q');
    assert(std::strcmp(view.line(0), "Qbc") == 0);

    view.clear();
    view.feed("one\ntwo", 7);
    assert(view.line_count() == 2);
    assert(std::strcmp(view.line(0), "one") == 0);
    assert(std::strcmp(view.line(1), "two") == 0);

    epui::TerminalView<8, 4> wrapped;
    wrapped.feed("abcde", 5);
    assert(wrapped.line_count() == 2);
    assert(std::strcmp(wrapped.line(0), "abcd") == 0);
    assert(std::strcmp(wrapped.line(1), "e") == 0);

    epui::TerminalView<8, 8> ansi;
    const char csi_start[] = "\x1b[";
    const char colored[] = "31mred\x1b[0m";
    ansi.feed(csi_start, sizeof(csi_start) - 1);
    ansi.feed(colored, sizeof(colored) - 1);
    assert(std::strcmp(ansi.line(0), "red") == 0);
    const char title[] = "\x1b]0;title\x07ok";
    ansi.feed(title, sizeof(title) - 1);
    assert(std::strcmp(ansi.line(0), "redok") == 0);
    ansi.feed('\x1b');
    ansi.clear();
    ansi.feed('A');
    assert(std::strcmp(ansi.line(0), "A") == 0);

    epui::TerminalView<3, 8> ring;
    ring.feed("one\ntwo\nthree\nfour", 18);
    assert(ring.line_count() == 3);
    assert(std::strcmp(ring.line(0), "two") == 0);
    assert(std::strcmp(ring.line(1), "three") == 0);
    assert(std::strcmp(ring.line(2), "four") == 0);
    ring.scroll(100);
    assert(ring.scroll_offset() == 2);
    ring.scroll(-1);
    assert(ring.scroll_offset() == 1);
    ring.feed('!');
    assert(ring.scroll_offset() == 0);
    assert(std::strcmp(ring.line(2), "four!") == 0);

    epui::TerminalView<2, 4> cursor;
    epui::Canvas canvas;
    cursor.draw(canvas, 0, 0, 24, 7, 0);
    assert(pixel_on(canvas, 0, 0));
    canvas.clear();
    cursor.draw(canvas, 0, 0, 24, 7, 600);
    assert(!pixel_on(canvas, 0, 0));
    cursor.set_cursor_visible(false);
    canvas.clear();
    cursor.draw(canvas, 0, 0, 24, 7, 0);
    assert(!pixel_on(canvas, 0, 0));

    return 0;
}
