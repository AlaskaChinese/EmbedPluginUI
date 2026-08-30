#pragma once

#include "epui/page.hpp"
#include "epui/terminal_controls.hpp"
#include "epui/terminal_line_editor.hpp"
#include "epui/terminal_view.hpp"

namespace epui::demo {

class TerminalDemoPage final : public Page {
public:
    explicit TerminalDemoPage(TerminalControls controls = TerminalControls{});
    void draw(Canvas& canvas, std::uint32_t now_ms) override;
    bool captures_key(Key key) const override;
    void on_key(Key key) override;
    void on_char(char ch) override;
    bool focused() const { return focused_; }
    const char* command() const { return editor_.command(); }
    std::size_t command_length() const { return editor_.length(); }
    std::size_t cursor() const { return editor_.cursor(); }
    const TerminalControls& controls() const { return controls_; }
    void set_controls(const TerminalControls& controls) { controls_ = controls; }
    const TerminalView<32, 21>& view() const { return view_; }
private:
    static constexpr std::size_t VisibleColumns = 20;

    void execute();

    TerminalView<32, 21> view_;
    TerminalControls controls_{};
    TerminalLineEditor<64, 8> editor_;
    bool focused_{false};
};

} // namespace epui::demo
