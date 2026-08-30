#pragma once

#include "epui/page.hpp"
#include "epui/popup_plugin.hpp"

namespace epui::demo {

class GraphicsDemoPage final : public Page {
public:
    explicit GraphicsDemoPage(PopupPlugin& popup) : popup_(popup) {}
    void draw(Canvas& canvas, std::uint32_t now_ms) override;
    bool captures_key(Key key) const override { return key == Key::Select; }
    void on_key(Key key) override;
    PopupResult last_result() const { return last_result_; }

private:
    static void popup_result(void* user, PopupResult result);

    PopupPlugin& popup_;
    PopupResult last_result_{PopupResult::Cancelled};
};

} // namespace epui::demo
