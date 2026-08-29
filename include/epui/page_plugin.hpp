#pragma once

#include "epui/page.hpp"
#include "epui/plugin.hpp"

namespace epui {

class PagePlugin : public Page, public Plugin {
public:
    PagePlugin(Ui& ui, const char* plugin_name) : ui_(ui), plugin_name_(plugin_name) {}

    const char* name() const final { return plugin_name_; }
    PluginKind kind() const final { return PluginKind::Page; }

    bool start() final {
        if (attached_) return true;
        if (!on_start()) return false;
        if (!ui_.add_page(*this)) {
            on_stop();
            return false;
        }
        attached_ = true;
        return true;
    }

    void stop() final {
        if (!attached_) return;
        ui_.remove_page(*this);
        attached_ = false;
        on_stop();
    }

    bool attached() const { return attached_; }

protected:
    virtual bool on_start() { return true; }
    virtual void on_stop() {}
    Ui& ui() { return ui_; }

private:
    Ui& ui_;
    const char* plugin_name_;
    bool attached_{false};
};

} // namespace epui
