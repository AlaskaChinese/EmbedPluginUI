#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include "epui/canvas.hpp"
#include "epui/page.hpp"
#include "epui/plugin.hpp"

namespace epui {

struct Menu;
using MenuCallback = void (*)(void* user);

enum class MenuItemKind : std::uint8_t {
    Action,
    Submenu,
    Toggle,
    Value,
};

struct MenuItem {
    const char* label{"Item"};
    MenuItemKind kind{MenuItemKind::Action};
    const Menu* child{nullptr};
    MenuCallback callback{nullptr};
    void* user{nullptr};
    bool* toggle_value{nullptr};
    int* int_value{nullptr};
    int min_value{0};
    int max_value{100};
    int step{1};

    static constexpr MenuItem action(const char* label, MenuCallback callback = nullptr, void* user = nullptr) {
        MenuItem item{};
        item.label = label;
        item.kind = MenuItemKind::Action;
        item.callback = callback;
        item.user = user;
        return item;
    }

    static constexpr MenuItem submenu(const char* label, const Menu& menu) {
        MenuItem item{};
        item.label = label;
        item.kind = MenuItemKind::Submenu;
        item.child = &menu;
        return item;
    }

    static constexpr MenuItem toggle(const char* label, bool& value,
                                     MenuCallback callback = nullptr, void* user = nullptr) {
        MenuItem item{};
        item.label = label;
        item.kind = MenuItemKind::Toggle;
        item.toggle_value = &value;
        item.callback = callback;
        item.user = user;
        return item;
    }

    static constexpr MenuItem value(const char* label, int& value, int minimum, int maximum, int step = 1,
                                    MenuCallback callback = nullptr, void* user = nullptr) {
        MenuItem item{};
        item.label = label;
        item.kind = MenuItemKind::Value;
        item.int_value = &value;
        item.min_value = minimum;
        item.max_value = maximum;
        item.step = step > 0 ? step : 1;
        item.callback = callback;
        item.user = user;
        return item;
    }
};

struct Menu {
    const char* title{"Menu"};
    const MenuItem* items{nullptr};
    std::size_t count{0};
};

template <std::size_t N>
constexpr Menu make_menu(const char* title, const MenuItem (&items)[N]) {
    return {title, items, N};
}

enum class MenuSelectionStyle : std::uint8_t {
    Indicator,
    LiquidGlass,
};

struct MenuStyle {
    std::uint8_t content_top{16};
    std::uint8_t row_height{10};
    std::uint8_t visible_rows{4};
    std::uint8_t text_x{11};
    std::uint8_t right_margin{4};
    std::uint8_t indicator_x{3};
    std::uint8_t indicator_width{4};
    std::uint8_t indicator_height{7};
    MenuSelectionStyle selection_style{MenuSelectionStyle::Indicator};
    std::uint8_t glass_x{3};
    std::uint8_t glass_width{122};
    std::uint8_t glass_height{9};
    std::uint8_t glass_radius{4};
    std::uint8_t glass_sheen_height{2};
    std::uint8_t glass_max_stretch{5};
    std::uint8_t glass_text_safe_padding_x{1};
    float glass_stretch_per_velocity{0.35f};
    float spring_stiffness{0.22f};
    float spring_damping{0.35f};
    std::uint16_t max_frame_ms{48};
};

template <std::size_t MaxDepth = 12>
class MenuPagePlugin : public Page, public Plugin {
public:
    static_assert(MaxDepth > 0, "MenuPagePlugin requires MaxDepth > 0");

    MenuPagePlugin(Ui& ui, const Menu& root, const char* plugin_name = "menu",
                   MenuStyle style = MenuStyle{}, bool auto_focus = true)
        : ui_(ui), root_(root), name_(plugin_name), style_(style), focused_(auto_focus) {
        frames_[0] = {&root_, 0};
        depth_ = 1;
        sync_selection(true);
    }

    const char* name() const override { return name_; }
    PluginKind kind() const override { return PluginKind::Menu; }

    bool start() override {
        if (attached_) return true;
        if (!ui_.add_page(*this)) return false;
        attached_ = true;
        return true;
    }

    void stop() override {
        if (!attached_) return;
        ui_.remove_page(*this);
        attached_ = false;
    }

    bool attached() const { return attached_; }
    bool focused() const { return focused_; }
    bool editing() const { return editing_; }
    std::size_t depth() const { return depth_; }
    std::size_t selected_index() const { return current_frame().selected; }
    const Menu& current_menu() const { return *current_frame().menu; }
    float selection_position() const { return selection_.position; }
    float scroll_position() const { return scroll_.position; }
    float panel_position() const { return panel_.position; }
    MenuSelectionStyle selection_style() const { return style_.selection_style; }
    const MenuStyle& style() const { return style_; }

    void set_style(const MenuStyle& style) {
        style_ = style;
        sync_selection(true);
    }

    void set_selection_style(MenuSelectionStyle style) {
        style_.selection_style = style;
    }

    void focus() { focused_ = true; }
    void blur() { focused_ = false; editing_ = false; }

    void reset_to_root(bool focus_menu = true) {
        frames_[0] = {&root_, 0};
        depth_ = 1;
        editing_ = false;
        focused_ = focus_menu;
        sync_selection(true);
        reset_spring(panel_, 0.0f);
    }

    bool captures_key(Key key) const override {
        if (key == Key::Select) return true;
        return focused_ && (key == Key::Next || key == Key::Prev || key == Key::Back);
    }

    void on_key(Key key) override {
        if (!focused_) {
            if (key == Key::Select) focus();
            return;
        }

        if (key == Key::Next) {
            if (editing_) adjust_value(1);
            else move_selection(1);
        } else if (key == Key::Prev) {
            if (editing_) adjust_value(-1);
            else move_selection(-1);
        } else if (key == Key::Select) {
            activate_selected();
        } else if (key == Key::Back) {
            if (editing_) editing_ = false;
            else if (depth_ > 1) leave_submenu();
            else blur();
        }
    }

    bool move_selection(int direction) {
        Frame& frame = current_frame();
        if (!frame.menu || frame.menu->count == 0 || direction == 0) return false;
        const std::size_t count = frame.menu->count;
        if (direction > 0) frame.selected = (frame.selected + 1) % count;
        else frame.selected = (frame.selected + count - 1) % count;
        sync_selection(false);
        return true;
    }

    bool activate_selected() {
        Frame& frame = current_frame();
        if (!frame.menu || frame.menu->count == 0 || frame.selected >= frame.menu->count) return false;
        const MenuItem& item = frame.menu->items[frame.selected];
        switch (item.kind) {
            case MenuItemKind::Action:
                if (item.callback) item.callback(item.user);
                return true;
            case MenuItemKind::Submenu:
                return item.child ? enter_submenu(*item.child) : false;
            case MenuItemKind::Toggle:
                if (!item.toggle_value) return false;
                *item.toggle_value = !*item.toggle_value;
                if (item.callback) item.callback(item.user);
                return true;
            case MenuItemKind::Value:
                if (!item.int_value) return false;
                editing_ = !editing_;
                return true;
        }
        return false;
    }

    bool enter_submenu(const Menu& menu) {
        if (depth_ >= MaxDepth) return false;
        frames_[depth_++] = {&menu, 0};
        editing_ = false;
        sync_selection(true);
        reset_spring(panel_, static_cast<float>(Canvas::Width));
        panel_.target = 0.0f;
        return true;
    }

    bool leave_submenu() {
        if (depth_ <= 1) return false;
        --depth_;
        editing_ = false;
        sync_selection(true);
        reset_spring(panel_, -static_cast<float>(Canvas::Width));
        panel_.target = 0.0f;
        return true;
    }

    bool jelly_active() const {
        return spring_active(selection_) || spring_active(scroll_) || spring_active(panel_);
    }

    void draw(Canvas& canvas, std::uint32_t now_ms) override {
        advance_animations(now_ms);
        draw_current_menu(canvas, round_to_int(panel_.position));
    }

private:
    struct Frame {
        const Menu* menu{nullptr};
        std::size_t selected{0};
    };

    struct Spring {
        float position{0.0f};
        float target{0.0f};
        float velocity{0.0f};
    };

    static float absf(float value) { return value < 0.0f ? -value : value; }

    static int round_to_int(float value) {
        return static_cast<int>(value >= 0.0f ? value + 0.5f : value - 0.5f);
    }

    static void reset_spring(Spring& spring, float value) {
        spring.position = value;
        spring.target = value;
        spring.velocity = 0.0f;
    }

    static bool spring_active(const Spring& spring) {
        return absf(spring.target - spring.position) > 0.05f || absf(spring.velocity) > 0.05f;
    }

    Frame& current_frame() { return frames_[depth_ - 1]; }
    const Frame& current_frame() const { return frames_[depth_ - 1]; }

    void sync_selection(bool snap) {
        const Frame& frame = current_frame();
        const std::size_t rows = style_.visible_rows == 0 ? 1 : style_.visible_rows;
        const float selected = static_cast<float>(frame.selected * style_.row_height);
        std::size_t first = 0;
        if (frame.selected >= rows) first = frame.selected - rows + 1;
        const float scroll = static_cast<float>(first * style_.row_height);
        if (snap) {
            reset_spring(selection_, selected);
            reset_spring(scroll_, scroll);
        } else {
            selection_.target = selected;
            scroll_.target = scroll;
        }
    }

    void adjust_value(int direction) {
        Frame& frame = current_frame();
        if (!frame.menu || frame.menu->count == 0) return;
        const MenuItem& item = frame.menu->items[frame.selected];
        if (item.kind != MenuItemKind::Value || !item.int_value) return;
        const int step = item.step > 0 ? item.step : 1;
        const int delta = direction > 0 ? step : -step;
        const int old_value = *item.int_value;
        const int next = std::max(item.min_value, std::min(item.max_value, old_value + delta));
        *item.int_value = next;
        if (next != old_value && item.callback) item.callback(item.user);
    }

    void step_spring(Spring& spring, float dt) {
        spring.velocity += (spring.target - spring.position) * style_.spring_stiffness * dt;
        float damping = 1.0f - style_.spring_damping * dt;
        if (damping < 0.0f) damping = 0.0f;
        spring.velocity *= damping;
        spring.position += spring.velocity * dt;
        if (absf(spring.target - spring.position) < 0.01f && absf(spring.velocity) < 0.01f) {
            spring.position = spring.target;
            spring.velocity = 0.0f;
        }
    }

    void advance_animations(std::uint32_t now_ms) {
        if (!has_draw_time_) {
            last_draw_ms_ = now_ms;
            has_draw_time_ = true;
            return;
        }
        std::uint32_t elapsed = now_ms - last_draw_ms_;
        last_draw_ms_ = now_ms;
        if (elapsed > style_.max_frame_ms) elapsed = style_.max_frame_ms;
        while (elapsed > 0) {
            const std::uint32_t step_ms = elapsed > 8 ? 8 : elapsed;
            const float dt = static_cast<float>(step_ms) / 16.0f;
            step_spring(selection_, dt);
            step_spring(scroll_, dt);
            step_spring(panel_, dt);
            elapsed -= step_ms;
        }
    }

    void draw_current_menu(Canvas& canvas, int panel_x) {
        const Frame& frame = current_frame();
        const Menu& menu = *frame.menu;
        const char* title = menu.title ? menu.title : "Menu";
        canvas.text(panel_x + 3, 3, title);

        char level[12]{};
        std::snprintf(level, sizeof(level), "%cL%u", focused_ ? '*' : '-', static_cast<unsigned>(depth_));
        canvas.text(panel_x + Canvas::Width - canvas.text_width(level) - 3, 3, level);
        canvas.line(panel_x + 2, 12, panel_x + Canvas::Width - 3, 12);

        if (menu.count == 0) {
            canvas.text(panel_x + 10, style_.content_top + 8, "(EMPTY)");
            return;
        }

        const int selection_y = round_to_int(static_cast<float>(style_.content_top) + selection_.position - scroll_.position);
        draw_selection(canvas, panel_x, selection_y);

        for (std::size_t i = 0; i < menu.count; ++i) {
            const float y_float = static_cast<float>(style_.content_top + i * style_.row_height) - scroll_.position;
            const int y = round_to_int(y_float);
            if (y < 13 || y > Canvas::Height - 10) continue;
            const MenuItem& item = menu.items[i];
            draw_item_label(canvas, item, y, panel_x);
            draw_item_value(canvas, item, y, i == frame.selected, panel_x);
        }
    }

    void clear_text_safe_region(Canvas& canvas, int x, int y, int width) const {
        if (style_.selection_style != MenuSelectionStyle::LiquidGlass || width <= 0) return;
        const int padding = static_cast<int>(style_.glass_text_safe_padding_x);
        canvas.fill_rect(x - padding, y, width + padding * 2, 7, false);
    }

    void draw_item_label(Canvas& canvas, const MenuItem& item, int y, int panel_x) {
        const char* label = item.label ? item.label : "?";
        const int x = panel_x + static_cast<int>(style_.text_x);
        clear_text_safe_region(canvas, x, y, canvas.text_width(label));
        canvas.text(x, y, label);
    }

    void draw_selection(Canvas& canvas, int panel_x, int selection_y) {
        if (selection_y < 13 || selection_y > Canvas::Height - 9) return;
        if (style_.selection_style == MenuSelectionStyle::LiquidGlass) {
            draw_liquid_glass_selection(canvas, panel_x, selection_y);
            return;
        }
        if (focused_) {
            canvas.fill_round_rect(panel_x + style_.indicator_x, selection_y,
                                   style_.indicator_width, style_.indicator_height, 2, true);
        } else {
            canvas.round_rect(panel_x + style_.indicator_x, selection_y,
                              style_.indicator_width, style_.indicator_height, 2, true);
        }
    }

    void draw_liquid_glass_selection(Canvas& canvas, int panel_x, int selection_y) {
        const float relative_velocity = selection_.velocity - scroll_.velocity;
        int stretch = round_to_int(absf(relative_velocity) * style_.glass_stretch_per_velocity);
        stretch = std::max(0, std::min(stretch, static_cast<int>(style_.glass_max_stretch)));

        const int base_x = static_cast<int>(style_.glass_x);
        const int max_width = std::max(10, Canvas::Width - base_x - 2);
        int width = static_cast<int>(style_.glass_width) - stretch * 2;
        width = std::max(10, std::min(width, max_width));
        const int x = panel_x + base_x + stretch;
        const int height = static_cast<int>(style_.glass_height) + stretch;
        const int y = selection_y - 1 - stretch / 2;
        const int radius = std::max(1, std::min(static_cast<int>(style_.glass_radius), height / 2));

        if (focused_ && style_.glass_sheen_height > 0 && absf(relative_velocity) > 0.12f) {
            const int sheen_height = std::min(static_cast<int>(style_.glass_sheen_height), std::max(1, height - 4));
            const int sheen_y = relative_velocity >= 0.0f
                ? y + height - sheen_height - 2
                : y + 2;
            canvas.invert_rect(x + 2, sheen_y, std::max(1, width - 4), sheen_height);
        }

        canvas.round_rect(x, y, width, height, radius, true);
        if (focused_ && width > radius * 2 + 4) {
            canvas.line(x + radius + 2, y + 1, x + width - radius - 3, y + 1, true);
        }
    }

    void draw_item_value(Canvas& canvas, const MenuItem& item, int y, bool selected, int panel_x) {
        char value[16]{};
        const char* right = nullptr;
        switch (item.kind) {
            case MenuItemKind::Submenu:
                right = ">";
                break;
            case MenuItemKind::Toggle:
                right = item.toggle_value && *item.toggle_value ? "ON" : "OFF";
                break;
            case MenuItemKind::Value:
                if (item.int_value) {
                    std::snprintf(value, sizeof(value), selected && editing_ ? "%d*" : "%d", *item.int_value);
                    right = value;
                }
                break;
            case MenuItemKind::Action:
            default:
                break;
        }
        if (!right) return;
        int x = Canvas::Width - style_.right_margin - canvas.text_width(right);
        if (x < static_cast<int>(style_.text_x) + 30) x = static_cast<int>(style_.text_x) + 30;
        x += panel_x;
        clear_text_safe_region(canvas, x, y, canvas.text_width(right));
        canvas.text(x, y, right);
    }

    Ui& ui_;
    const Menu& root_;
    const char* name_;
    MenuStyle style_;
    Frame frames_[MaxDepth]{};
    std::size_t depth_{1};
    bool attached_{false};
    bool focused_{true};
    bool editing_{false};
    Spring selection_{};
    Spring scroll_{};
    Spring panel_{};
    std::uint32_t last_draw_ms_{0};
    bool has_draw_time_{false};
};

} // namespace epui
