#pragma once

#include "epui/page.hpp"
#include "epui/terminal_view.hpp"

namespace epui::demo {

class TerminalDemoPage final : public Page {
public:
    TerminalDemoPage();
    void draw(Canvas& canvas, std::uint32_t now_ms) override;
    bool captures_key(Key key) const override;
    void on_key(Key key) override;
    void on_char(char ch) override;
    bool focused() const { return focused_; }
    const TerminalView<32, 21>& view() const { return view_; }
private:
    TerminalView<32, 21> view_;
    bool focused_{false};
};

} // namespace epui::demo
