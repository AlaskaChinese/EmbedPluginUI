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

    // Legacy layout fields remain source-compatible. New projects should use
    // content_inset_left/right because those are measured from the selection
    // frame itself and make symmetric layouts straightforward.
    std::uint8_t text_x{11};
    std::uint8_t right_margin{4};
    std::uint8_t content_inset_left{8};
    std::uint8_t content_inset_right{8};

    std::uint8_t indicator_x{3};
    std::uint8_t indicator_width{4};
    std::uint8_t indicator_height{7};
    MenuSelectionStyle selection_style{MenuSelectionStyle::Indicator};

    // OLED-native selection frame geometry.
    std::uint8_t glass_x{3};
    std::uint8_t glass_width{122};
    std::uint8_t glass_height{9};
    std::uint8_t glass_radius{4};
    std::uint8_t glass_sheen_height{1};
    std::uint8_t glass_max_stretch{4};
    float glass_stretch_per_velocity{0.25f};

    // Metaball geometry. The effect is drawn only around the two capsule ends,
    // so the text band remains readable on a 1-bit display.
    std::uint8_t liquid_metaball_radius{3};
    std::uint8_t liquid_bridge_width{1};
    std::uint8_t liquid_bridge_max_span{18};
    std::uint8_t liquid_refraction_px{1};
    std::uint8_t liquid_refraction_radius{5};
    std::uint8_t liquid_highlight_min{6};
    std::uint8_t liquid_highlight_max{20};
    std::uint8_t liquid_trail_length{4};
    float liquid_motion_threshold{0.12f};
    bool liquid_dither_trail{false};

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
    static float clamp01(float value) {
        if (value < 0.0f) return 0.0f;
        if (value > 1.0f) return 1.0f;
        return value;
    }
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

    int content_left_x() const {
        return std::min(Canvas::Width - 1,
                        static_cast<int>(style_.glass_x) + static_cast<int>(style_.content_inset_left));
    }

    int content_right_edge() const {
        int edge = static_cast<int>(style_.glass_x) + static_cast<int>(style_.glass_width) - 1
                 - static_cast<int>(style_.content_inset_right);
        edge = std::min(edge, Canvas::Width - 1);
        return std::max(content_left_x(), edge);
    }

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
        int x = content_right_edge() - canvas.text_width(right) + 1;
        if (x < content_left_x() + 30) x = content_left_x() + 30;
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
            canvas.text(panel_x + content_left_x(), y, item.label ? item.label : "?");
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

    void fill_disc(Canvas& canvas, int cx, int cy, int radius, bool on = true) const {
        if (radius <= 0) return;
        const int rr = radius * radius;
        for (int dy = -radius; dy <= radius; ++dy) {
            int half = 0;
            while ((half + 1) * (half + 1) + dy * dy <= rr) ++half;
            canvas.line(cx - half, cy + dy, cx + half, cy + dy, on);
        }
    }

    void draw_metaball_pair(Canvas& canvas, int cx, int y0, int y1,
                            int r0, int r1, int neck) const {
        if (r0 <= 0 || r1 <= 0) return;
        fill_disc(canvas, cx, y0, r0, true);
        fill_disc(canvas, cx, y1, r1, true);

        const int delta = y1 - y0;
        const int span = absi(delta);
        if (span <= 1 || span > static_cast<int>(style_.liquid_bridge_max_span)) return;
        const int direction = delta > 0 ? 1 : -1;
        const int safe_neck = std::max(1, neck);

        for (int i = 1; i < span; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(span);
            const float edge = absf(t * 2.0f - 1.0f);
            const float end_radius = t < 0.5f ? static_cast<float>(r0) : static_cast<float>(r1);
            int half = round_to_int(static_cast<float>(safe_neck)
                + (end_radius - static_cast<float>(safe_neck)) * edge);
            half = std::max(safe_neck, half);
            const int y = y0 + direction * i;
            canvas.line(cx - half, y, cx + half, y, true);
        }
    }

    float liquid_progress() const {
        const float total = absf(selection_.target - liquid_anchor_position_);
        if (total < 0.01f) return 1.0f;
        return clamp01(absf(selection_.position - liquid_anchor_position_) / total);
    }

    void draw_metaball_motion(Canvas& canvas, int x, int y, int width, int height) const {
        const float speed = absf(selection_.velocity - scroll_.velocity);
        if (speed <= style_.liquid_motion_threshold) return;

        const int current_center_y = y + height / 2;
        const int anchor_selection_y = round_to_int(
            static_cast<float>(style_.content_top) + liquid_anchor_position_ - scroll_.position);
        const int anchor_center_y = anchor_selection_y - 1 + height / 2;
        const int target_selection_y = round_to_int(
            static_cast<float>(style_.content_top) + selection_.target - scroll_.position);
        const int target_center_y = target_selection_y - 1 + height / 2;
        const int radius = std::max(2, static_cast<int>(style_.liquid_metaball_radius));
        const int neck = std::max(1, static_cast<int>(style_.liquid_bridge_width));
        const int left_cx = x + radius;
        const int right_cx = x + width - radius - 1;
        const float progress = liquid_progress();

        if (progress < 0.5f) {
            const float local = progress * 2.0f;
            const int source_radius = std::max(1, round_to_int(static_cast<float>(radius) * (1.0f - 0.55f * local)));
            draw_metaball_pair(canvas, left_cx, anchor_center_y, current_center_y,
                               source_radius, radius, neck);
            draw_metaball_pair(canvas, right_cx, anchor_center_y, current_center_y,
                               source_radius, radius, neck);
        } else {
            const float local = (progress - 0.5f) * 2.0f;
            const int target_radius = std::max(1, round_to_int(static_cast<float>(radius) * (0.45f + 0.55f * local)));
            draw_metaball_pair(canvas, left_cx, current_center_y, target_center_y,
                               radius, target_radius, neck);
            draw_metaball_pair(canvas, right_cx, current_center_y, target_center_y,
                               radius, target_radius, neck);
        }

        if (style_.liquid_dither_trail) {
            const int direction = selection_.velocity >= 0.0f ? -1 : 1;
            const int trail = static_cast<int>(style_.liquid_trail_length);
            for (int i = 2; i <= trail; i += 2) {
                canvas.pixel(left_cx, current_center_y + direction * i, true);
                canvas.pixel(right_cx, current_center_y + direction * i, true);
            }
        }
    }

    void draw_liquid_highlight(Canvas& canvas, int x, int y, int width, int height,
                               int radius, float relative_velocity, float speed) const {
        if (!focused_ || style_.glass_sheen_height == 0 || speed <= style_.liquid_motion_threshold) return;
        const int min_length = static_cast<int>(style_.liquid_highlight_min);
        const int max_length = std::max(min_length, static_cast<int>(style_.liquid_highlight_max));
        int length = min_length + round_to_int(speed * 2.5f);
        length = std::max(2, std::min(length, max_length));
        length = std::min(length, std::max(2, width - radius * 2 - 4));
        const int start = relative_velocity >= 0.0f
            ? x + radius + 2
            : x + width - radius - 2 - length;
        const int edge_y = relative_velocity >= 0.0f ? y + height : y - 1;
        canvas.line(start, edge_y, start + length - 1, edge_y, true);
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
        draw_metaball_motion(canvas, x, y, width, height);
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
            const int label_x = panel_x + content_left_x();
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
