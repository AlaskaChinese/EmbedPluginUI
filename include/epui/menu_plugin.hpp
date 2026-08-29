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

    // OLED-native liquid capsule geometry. The legacy glass_* names remain
    // source-compatible, but the effect no longer depends on alpha or blur.
    std::uint8_t glass_x{3};
    std::uint8_t glass_width{122};
    std::uint8_t glass_height{9};
    std::uint8_t glass_radius{4};
    std::uint8_t glass_sheen_height{1};
    std::uint8_t glass_max_stretch{5};
    float glass_stretch_per_velocity{0.35f};

    std::uint8_t liquid_bridge_width{2};
    std::uint8_t liquid_bridge_max_span{18};
    std::uint8_t liquid_refraction_px{1};
    std::uint8_t liquid_refraction_radius{6};
    std::uint8_t liquid_highlight_min{8};
    std::uint8_t liquid_highlight_max{28};
    std::uint8_t liquid_trail_length{6};
    float liquid_motion_threshold{0.12f};
    bool liquid_dither_trail{true};

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
    float liquid_anchor_position() const { return liquid_anchor_position_; }
    MenuSelectionStyle selection_style() const { return style_.selection_style; }
    const MenuStyle& style() const { return style_; }

    void set_style(const MenuStyle& style) {
        style_ = style;
        sync_selection(true);
    }

    void set_selection_style(MenuSelectionStyle style) {
        style_.selection_style = style;
        if (style == MenuSelectionStyle::LiquidGlass) liquid_anchor_position_ = selection_.position;
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
    static int absi(int value) { return value < 0 ? -value : value; }
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
            liquid_anchor_position_ = selected;
        } else {
            liquid_anchor_position_ = selection_.position;
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
        if (!spring_active(selection_)) liquid_anchor_position_ = selection_.target;
    }

    const char* format_item_value(const MenuItem& item, bool selected,
                                  char* value, std::size_t value_size) const {
        switch (item.kind) {
            case MenuItemKind::Submenu:
                return ">";
            case MenuItemKind::Toggle:
                return item.toggle_value && *item.toggle_value ? "ON" : "OFF";
            case MenuItemKind::Value:
                if (item.int_value) {
                    std::snprintf(value, value_size, selected && editing_ ? "%d*" : "%d", *item.int_value);
                    return value;
                }
                return nullptr;
            case MenuItemKind::Action:
            default:
                return nullptr;
        }
    }

    void draw_item_value(Canvas& canvas, const MenuItem& item, int y, bool selected,
                         int panel_x, int x_offset = 0, bool on = true) const {
        char value[16]{};
        const char* right = format_item_value(item, selected, value, sizeof(value));
        if (!right) return;
        int x = Canvas::Width - style_.right_margin - canvas.text_width(right);
        if (x < static_cast<int>(style_.text_x) + 30) x = static_cast<int>(style_.text_x) + 30;
        canvas.text(panel_x + x + x_offset, y, right, on);
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

        for (std::size_t i = 0; i < menu.count; ++i) {
            const float y_float = static_cast<float>(style_.content_top + i * style_.row_height) - scroll_.position;
            const int y = round_to_int(y_float);
            if (y < 13 || y > Canvas::Height - 10) continue;
            const MenuItem& item = menu.items[i];
            canvas.text(panel_x + style_.text_x, y, item.label ? item.label : "?");
            draw_item_value(canvas, item, y, i == frame.selected, panel_x);
        }

        const int selection_y = round_to_int(static_cast<float>(style_.content_top) + selection_.position - scroll_.position);
        draw_selection(canvas, panel_x, selection_y);
        if (style_.selection_style == MenuSelectionStyle::LiquidGlass) {
            apply_liquid_refraction(canvas, menu, panel_x, selection_y);
        }
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

    void draw_liquid_bridge(Canvas& canvas, int x, int y, int width, int height,
                            int radius, int anchor_selection_y, float speed) const {
        const int anchor_y = anchor_selection_y - 1;
        const int current_center = y + height / 2;
        const int anchor_center = anchor_y + height / 2;
        const int delta = anchor_center - current_center;
        if (absi(delta) < 2) return;

        const int direction = delta > 0 ? 1 : -1;
        int current_edge = direction > 0 ? y + height - 1 : y;
        int anchor_edge = direction > 0 ? anchor_y : anchor_y + height - 1;
        if (absi(anchor_edge - current_edge) > static_cast<int>(style_.liquid_bridge_max_span)) {
            anchor_edge = current_edge + direction * static_cast<int>(style_.liquid_bridge_max_span);
        }

        const int bridge_width = std::max(1, static_cast<int>(style_.liquid_bridge_width));
        const int inset = std::max(2, radius / 2 + 1);
        const int left_x = x + inset;
        const int right_x = x + width - inset - bridge_width;

        for (int i = 0; i < bridge_width; ++i) {
            canvas.line(left_x + i, current_edge, left_x + i, anchor_edge, true);
            canvas.line(right_x + i, current_edge, right_x + i, anchor_edge, true);
        }

        const int lobe_y = anchor_edge - (direction > 0 ? 0 : 2);
        canvas.fill_round_rect(left_x - 1, lobe_y, bridge_width + 2, 3, 1, true);
        canvas.fill_round_rect(right_x - 1, lobe_y, bridge_width + 2, 3, 1, true);

        if (!style_.liquid_dither_trail || speed <= style_.liquid_motion_threshold) return;
        const int trail = std::min(static_cast<int>(style_.liquid_trail_length), absi(anchor_edge - current_edge));
        for (int i = 1; i <= trail; ++i) {
            if ((i & 1) == 0) continue;
            const int yy = current_edge + direction * i;
            canvas.pixel(left_x - 2, yy, true);
            canvas.pixel(right_x + bridge_width + 1, yy, true);
        }
    }

    void draw_liquid_pull_lobes(Canvas& canvas, int x, int width, int height,
                                int target_selection_y, int current_selection_y) const {
        const int delta = target_selection_y - current_selection_y;
        if (absi(delta) < 2) return;
        const int target_y = target_selection_y - 1;
        const bool target_below = delta > 0;
        const int lobe_y = target_below ? target_y : target_y + height - 2;
        canvas.fill_round_rect(x + 1, lobe_y, 4, 2, 1, true);
        canvas.fill_round_rect(x + width - 5, lobe_y, 4, 2, 1, true);
    }

    void draw_liquid_highlight(Canvas& canvas, int x, int y, int width, int height,
                               int radius, float relative_velocity, float speed) const {
        if (!focused_ || style_.glass_sheen_height == 0 || speed <= style_.liquid_motion_threshold) return;
        const int min_length = static_cast<int>(style_.liquid_highlight_min);
        const int max_length = std::max(min_length, static_cast<int>(style_.liquid_highlight_max));
        int length = min_length + round_to_int(speed * 4.0f);
        length = std::max(2, std::min(length, max_length));
        length = std::min(length, std::max(2, width - radius * 2 - 4));

        const int available = std::max(0, width - radius * 2 - length - 4);
        int bias = round_to_int(speed * 2.0f);
        bias = std::max(0, std::min(bias, available));
        const int start = relative_velocity >= 0.0f
            ? x + radius + 2 + bias
            : x + width - radius - 2 - length - bias;
        const int edge_y = relative_velocity >= 0.0f ? y + height : y - 1;
        const int lines = std::min(2, static_cast<int>(style_.glass_sheen_height));
        for (int i = 0; i < lines; ++i) {
            const int yy = edge_y + (relative_velocity >= 0.0f ? i : -i);
            canvas.line(start, yy, start + length - 1, yy, true);
        }
    }

    void draw_liquid_glass_selection(Canvas& canvas, int panel_x, int selection_y) {
        const float relative_velocity = selection_.velocity - scroll_.velocity;
        const float speed = absf(relative_velocity);
        int squeeze = round_to_int(speed * style_.glass_stretch_per_velocity);
        squeeze = std::max(0, std::min(squeeze, static_cast<int>(style_.glass_max_stretch)));

        const int base_x = static_cast<int>(style_.glass_x);
        const int max_width = std::max(10, Canvas::Width - base_x - 2);
        int width = static_cast<int>(style_.glass_width) - squeeze * 2;
        width = std::max(10, std::min(width, max_width));
        const int x = panel_x + base_x + squeeze;
        const int height = std::max(5, static_cast<int>(style_.glass_height));
        const int y = selection_y - 1;
        const int radius = std::max(1, std::min(static_cast<int>(style_.glass_radius), height / 2));

        canvas.round_rect(x, y, width, height, radius, true);

        // While moving, break the long horizontal edges into side caps. On a
        // 1-bit OLED this reads as a deforming membrane instead of a rigid box.
        if (speed > style_.liquid_motion_threshold && width > radius * 2 + 20) {
            const int cap = std::min(8, std::max(3, width / 12));
            const int erase_x = x + radius + cap;
            const int erase_w = width - (radius + cap) * 2;
            if (erase_w > 0) {
                canvas.line(erase_x, y, erase_x + erase_w - 1, y, false);
                canvas.line(erase_x, y + height - 1, erase_x + erase_w - 1, y + height - 1, false);
            }
        }

        const int anchor_selection_y = round_to_int(
            static_cast<float>(style_.content_top) + liquid_anchor_position_ - scroll_.position);
        draw_liquid_bridge(canvas, x, y, width, height, radius, anchor_selection_y, speed);

        const int target_selection_y = round_to_int(
            static_cast<float>(style_.content_top) + selection_.target - scroll_.position);
        draw_liquid_pull_lobes(canvas, x, width, height, target_selection_y, selection_y);
        draw_liquid_highlight(canvas, x, y, width, height, radius, relative_velocity, speed);
    }

    void apply_liquid_refraction(Canvas& canvas, const Menu& menu, int panel_x, int selection_y) const {
        const float relative_velocity = selection_.velocity - scroll_.velocity;
        const float speed = absf(relative_velocity);
        if (speed <= style_.liquid_motion_threshold || style_.liquid_refraction_px == 0) return;

        const int radius = static_cast<int>(style_.liquid_refraction_radius);
        const int amount = static_cast<int>(style_.liquid_refraction_px);
        const Frame& frame = current_frame();

        for (std::size_t i = 0; i < menu.count; ++i) {
            const int y = round_to_int(static_cast<float>(style_.content_top + i * style_.row_height) - scroll_.position);
            if (y < 13 || y > Canvas::Height - 10) continue;
            const int distance = y - selection_y;
            if (absi(distance) > radius) continue;

            int offset = amount;
            if (distance < 0) offset = -amount;
            else if (distance == 0 && relative_velocity < 0.0f) offset = -amount;

            const MenuItem& item = menu.items[i];
            const char* label = item.label ? item.label : "?";
            const int label_x = panel_x + static_cast<int>(style_.text_x);
            canvas.text(label_x, y, label, false);
            draw_item_value(canvas, item, y, i == frame.selected, panel_x, 0, false);
            canvas.text(label_x + offset, y, label, true);
            draw_item_value(canvas, item, y, i == frame.selected, panel_x, offset, true);
        }
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
    float liquid_anchor_position_{0.0f};
    std::uint32_t last_draw_ms_{0};
    bool has_draw_time_{false};
};

} // namespace epui
