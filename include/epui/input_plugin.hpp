#pragma once

#include <cstddef>
#include "epui/page.hpp"
#include "epui/plugin.hpp"

namespace epui {

struct InputEvent {
    Key key{Key::Select};
    bool pressed{true};
};

class InputPlugin : public Plugin {
public:
    PluginKind kind() const final { return PluginKind::Input; }
    virtual bool poll(InputEvent& event) = 0;
};

class CallbackInputPlugin final : public InputPlugin {
public:
    using PollFn = bool (*)(void* user, InputEvent& event);
    CallbackInputPlugin(const char* plugin_name, void* user, PollFn poll)
        : name_(plugin_name), user_(user), poll_(poll) {}
    const char* name() const override { return name_ ? name_ : "callback-input"; }
    bool poll(InputEvent& event) override { return poll_ && poll_(user_, event); }
private:
    const char* name_{};
    void* user_{};
    PollFn poll_{};
};

template <std::size_t Capacity = 8>
class QueuedInputPlugin final : public InputPlugin {
public:
    explicit QueuedInputPlugin(const char* plugin_name = "queued-input") : name_(plugin_name) {}
    const char* name() const override { return name_; }
    bool push(InputEvent event) {
        if (count_ >= Capacity) return false;
        queue_[(head_ + count_) % Capacity] = event;
        ++count_;
        return true;
    }
    bool poll(InputEvent& event) override {
        if (count_ == 0) return false;
        event = queue_[head_];
        head_ = (head_ + 1) % Capacity;
        --count_;
        return true;
    }
private:
    const char* name_;
    InputEvent queue_[Capacity]{};
    std::size_t head_{0};
    std::size_t count_{0};
};

} // namespace epui
