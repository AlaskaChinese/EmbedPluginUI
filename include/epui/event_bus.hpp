#pragma once

#include <cstddef>
#include "epui/plugin.hpp"

namespace epui {

class EventBusPlugin final : public Plugin {
public:
    static constexpr std::size_t MaxSubscriptions = 16;

    explicit EventBusPlugin(const char* plugin_name = "event-bus") : name_(plugin_name) {}

    const char* name() const override { return name_; }
    PluginKind kind() const override { return PluginKind::Service; }

    template <typename Event, void (*Handler)(void* user, const Event& event)>
    bool subscribe(void* owner) {
        if (count_ >= MaxSubscriptions) return false;
        const void* token = type_token<Event>();
        for (std::size_t i = 0; i < count_; ++i) {
            if (subscriptions_[i].owner == owner && subscriptions_[i].type == token &&
                subscriptions_[i].invoke == &invoke<Event, Handler>) return false;
        }
        subscriptions_[count_++] = {token, owner, &invoke<Event, Handler>};
        return true;
    }

    void unsubscribe_owner(void* owner) {
        std::size_t write = 0;
        for (std::size_t read = 0; read < count_; ++read) {
            if (subscriptions_[read].owner == owner) continue;
            if (write != read) subscriptions_[write] = subscriptions_[read];
            ++write;
        }
        count_ = write;
    }

    template <typename Event>
    std::size_t publish(const Event& event) const {
        std::size_t delivered = 0;
        const void* token = type_token<Event>();
        for (std::size_t i = 0; i < count_; ++i) {
            const Subscription& s = subscriptions_[i];
            if (s.type != token) continue;
            s.invoke(s.owner, &event);
            ++delivered;
        }
        return delivered;
    }

    std::size_t subscription_count() const { return count_; }

private:
    using Invoker = void (*)(void*, const void*);

    struct Subscription {
        const void* type{};
        void* owner{};
        Invoker invoke{};
    };

    template <typename Event>
    static const void* type_token() {
        static const unsigned char token = 0;
        return &token;
    }

    template <typename Event, void (*Handler)(void*, const Event&)>
    static void invoke(void* owner, const void* event) {
        Handler(owner, *static_cast<const Event*>(event));
    }

    const char* name_;
    Subscription subscriptions_[MaxSubscriptions]{};
    std::size_t count_{0};
};

} // namespace epui
