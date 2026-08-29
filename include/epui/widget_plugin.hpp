#pragma once

#include <cstddef>
#include "epui/canvas.hpp"
#include "epui/page_plugin.hpp"
#include "epui/plugin.hpp"

namespace epui {

class WidgetPlugin : public Plugin {
public:
    PluginKind kind() const final { return PluginKind::Widget; }
    virtual void draw(Canvas& canvas, int x, int y, std::uint32_t now_ms) = 0;

    void set_visible(bool visible) { visible_ = visible; }
    bool visible() const { return visible_; }

private:
    bool visible_{true};
};

template <std::size_t MaxWidgets = 8>
class WidgetPagePlugin : public PagePlugin {
public:
    WidgetPagePlugin(Ui& ui, const char* page_name) : PagePlugin(ui, page_name) {}

    bool add_widget(WidgetPlugin& widget, int x, int y) {
        if (count_ >= MaxWidgets) return false;
        placements_[count_] = {&widget, x, y};
        dependency_names_[count_] = widget.name();
        ++count_;
        return true;
    }

    PluginDependencies dependencies() const override {
        return {dependency_names_, count_};
    }

    void draw(Canvas& canvas, std::uint32_t now_ms) override {
        before_widgets(canvas, now_ms);
        for (std::size_t i = 0; i < count_; ++i) {
            const Placement& placement = placements_[i];
            if (placement.widget && placement.widget->visible()) {
                placement.widget->draw(canvas, placement.x, placement.y, now_ms);
            }
        }
        after_widgets(canvas, now_ms);
    }

protected:
    virtual void before_widgets(Canvas&, std::uint32_t) {}
    virtual void after_widgets(Canvas&, std::uint32_t) {}

private:
    struct Placement {
        WidgetPlugin* widget{};
        int x{0};
        int y{0};
    };

    Placement placements_[MaxWidgets]{};
    const char* dependency_names_[MaxWidgets]{};
    std::size_t count_{0};
};

} // namespace epui
