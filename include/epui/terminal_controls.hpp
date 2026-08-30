#pragma once

#include <cstdint>
#include "epui/page.hpp"

namespace epui {

enum class TerminalAction : std::uint8_t {
    Ignore,
    Focus,
    Unfocus,
    Execute,
    CursorLeft,
    CursorRight,
    HistoryPrevious,
    HistoryNext,
    OutputUp,
    OutputDown,
};

struct TerminalControls {
    Key focus{Key::Select};
    Key unfocus{Key::Back};
    Key execute{Key::Select};
    Key cursor_left{Key::Left};
    Key cursor_right{Key::Right};
    Key history_previous{Key::Up};
    Key history_next{Key::Down};
    Key output_up{Key::ScrollUp};
    Key output_down{Key::ScrollDown};

    TerminalAction action_for(Key key, bool focused) const {
        if (!focused) return key == focus ? TerminalAction::Focus : TerminalAction::Ignore;
        if (key == unfocus) return TerminalAction::Unfocus;
        if (key == execute) return TerminalAction::Execute;
        if (key == cursor_left) return TerminalAction::CursorLeft;
        if (key == cursor_right) return TerminalAction::CursorRight;
        if (key == history_previous) return TerminalAction::HistoryPrevious;
        if (key == history_next) return TerminalAction::HistoryNext;
        if (key == output_up) return TerminalAction::OutputUp;
        if (key == output_down) return TerminalAction::OutputDown;
        return TerminalAction::Ignore;
    }

    bool captures(Key key, bool focused) const {
        return action_for(key, focused) != TerminalAction::Ignore;
    }
};

} // namespace epui
