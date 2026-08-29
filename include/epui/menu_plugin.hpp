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
    Choice,
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
    const char* const* choice_options{nullptr};
    std::size_t choice_count{0};
    std::uint8_t* choice_index{nullptr};

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

    template <std::size_t N>
    static constexpr MenuItem choice(const char* label, std::uint8_t& index,
                                     const char* const (&options)[N],
                                     MenuCallback callback = nullptr, void* user = nullptr) {
        MenuItem item{};
        item.label = label;
        item.kind = MenuItemKind::Choice;
        item.choice_options = options;
        item.choice_count = N;
        item.choice_index = &index;
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
    GlideFrame,
    SlideFrame,
    LiquidGlass,
};

struct MenuStyle {
    std::uint8_t content_top{16};
    std::uint8_t row_height{10};
    std::uint8_t visible_rows{4};

    // Legacy layout fields remain source-compatible. New projects should use
    // content_inset_left/right, measured from the selection frame itself.
    std::uint8_t text_x{11};
    std::uint8_t right_margin{4};
    std::uint8_t content_inset_left{8};
    std::uint8_t content_inset_right{8};

    std::uint8_t indicator_x{3};
    std::uint8_t indicator_width{4};
    std::uint8_t indicator_height{7};
    MenuSelectionStyle selection_style{MenuSelectionStyle::Indicator};

    // Shared rounded-frame geometry. 11 px leaves two pixels above and below
    // the built-in 5x7 text in its resting state.
    std::uint8_t glass_x{3};
    std::uint8_t glass_width{122};
    std::uint8_t glass_height{11};
    std::uint8_t glass_radius{4};

    // GlideFrame generates a deterministic small-OLED target path: move
    // quickly while far from the target, then approach one pixel per logical
    // tick. A real damped spring follows that path for the visible frame.
    // SlideFrame uses the same target path but keeps full frame width.
    bool glide_fit_content{true};
    std::uint8_t glide_min_width{24};
    std::uint8_t glide_position_fast_step{5};
    std::uint8_t glide_position_slow_zone{4};
    std::uint8_t glide_width_fast_step{10};
    std::uint8_t glide_width_slow_zone{5};
    std::uint8_t glide_scroll_fast_step{4};
    std::uint8_t glide_scroll_slow_zone{4};
    std::uint16_t glide_tick_ms{16};

    // Deprecated source-compatibility fields from the short-lived separate
    // kick-spring implementation. They are intentionally no longer consumed.
    std::uint8_t frame_jelly_kick{2};
    std::uint8_t frame_jelly_max_stretch{2};
    float frame_jelly_stiffness{0.34f};
    float frame_jelly_damping{0.28f};

    // Shared jelly geometry for GlideFrame, SlideFrame and LiquidGlass.
    // The actual visible spring velocity squeezes the frame horizontally and
    // stretches it vertically. LiquidGlass alone adds the moving sheen.
    std::uint8_t glass_sheen_height{2};
    std::uint8_t glass_max_stretch{5};
    float glass_stretch_per_velocity{0.35f};
    float glass_motion_threshold{0.12f};

    // When selection crosses a viewport edge, the list scrolls while the
    // rounded frame stays anchored to the edge. This spring kick gives the
    // anchored frame a visible push/rebound instead of making it look frozen.
    float scroll_handoff_kick{2.0f};

    // One spring model is used by Indicator/LiquidGlass and by the visible
    // rounded-frame follower used by GlideFrame/SlideFrame.
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
    std::size_t first_visible_index() const { return current_frame().first_visible; }
    const Menu& current_menu() const { return *current_frame().menu; }
    float selection_position() const { return selection_.position; }
    float scroll_position() const { return scroll_.position; }
    float panel_position() const { return panel_.position; }
    int glide_position() const { return glide_position_; }
    int glide_target_position() const { return glide_position_target_; }
    int glide_width() const { return glide_width_; }
    int glide_target_width() const { return glide_width_target_; }
    int glide_scroll_position() const { return glide_scroll_position_; }
    int glide_scroll_target() const { return glide_scroll_target_; }

    // For Glide/Slide this is the visible spring that follows the deterministic
    // glide path. For Glass it is the existing selection-minus-scroll spring.
    // Rounded frames also add the shared viewport-edge handoff spring.
    float frame_motion_position() const {
        const float base = is_glide_style()
            ? glide_frame_.position : selection_.position - scroll_.position;
        return base + (is_rounded_frame_style() ? edge_handoff_.position : 0.0f);
    }
    float frame_motion_target() const {
        const float base = is_glide_style()
            ? glide_frame_.target : selection_.target - scroll_.target;
        return base + (is_rounded_frame_style() ? edge_handoff_.target : 0.0f);
    }
    float frame_motion_velocity() const {
        const float base = is_glide_style()
            ? glide_frame_.velocity : selection_.velocity - scroll_.velocity;
        return base + (is_rounded_frame_style() ? edge_handoff_.velocity : 0.0f);
    }
    // Kept for source compatibility with the short-lived kick-spring API.
    // It reports the signed shared velocity-driven deformation amount.
    float frame_jelly_offset() const {
        return frame_motion_velocity() * style_.glass_stretch_per_velocity;
    }

    MenuSelectionStyle selection_style() const { return style_.selection_style; }
    const MenuStyle& style() const { return style_; }

    void set_style(const MenuStyle& style) {
        style_ = style;
        sync_selection(true);
        glide_width_initialized_ = false;
    }

    void set_selection_style(MenuSelectionStyle style) {
        style_.selection_style = style;
        sync_selection(true);
        glide_width_initialized_ = false;
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
        glide_width_initialized_ = false;
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
        const std::size_t old_selected = frame.selected;
        if (direction > 0) frame.selected = (frame.selected + 1) % count;
        else frame.selected = (frame.selected + count - 1) % count;

        const bool wrapped = (direction > 0 && old_selected + 1 >= count)
            || (direction < 0 && old_selected == 0);
        sync_selection(false, wrapped ? 0 : direction);
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
            case MenuItemKind::Choice:
                if (!item.choice_index || !item.choice_options || item.choice_count == 0) return false;
                *item.choice_index = static_cast<std::uint8_t>(
                    (static_cast<std::size_t>(*item.choice_index) + 1u) % item.choice_count);
                if (item.callback) item.callback(item.user);
                return true;
        }
        return false;
    }

    bool enter_submenu(const Menu& menu) {
        if (depth_ >= MaxDepth) return false;
        frames_[depth_++] = {&menu, 0};
        editing_ = false;
        sync_selection(true);
        glide_width_initialized_ = false;
        reset_spring(panel_, static_cast<float>(Canvas::Width));
        panel_.target = 0.0f;
        return true;
    }

    bool leave_submenu() {
        if (depth_ <= 1) return false;
        --depth_;
        editing_ = false;
        sync_selection(true);
        glide_width_initialized_ = false;
        reset_spring(panel_, -static_cast<float>(Canvas::Width));
        panel_.target = 0.0f;
        return true;
    }

    bool jelly_active() const {
        const bool glide_jelly = is_glide_style()
            && (spring_active(glide_frame_) || glide_active());
        const bool handoff_jelly = is_rounded_frame_style() && spring_active(edge_handoff_);
        return spring_active(selection_) || spring_active(scroll_) || spring_active(panel_)
            || glide_jelly || handoff_jelly;
    }

    void draw(Canvas& canvas, std::uint32_t now_ms) override {
        update_glide_width_target(canvas);
        advance_animations(now_ms);
        draw_current_menu(canvas, round_to_int(panel_.position));
    }

private:
    struct Frame {
        const Menu* menu{nullptr};
        std::size_t selected{0};
        std::size_t first_visible{0};
    };

    struct Spring {
        float position{0.0f};
        float target{0.0f};
        float velocity{0.0f};
    };

    static constexpr int ContentClipTop = 13;

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

    static bool glide_toward(int& value, int target, int fast_step, int slow_zone) {
        const int difference = target - value;
        if (difference == 0) return false;
        const int distance = absi(difference);
        int step = distance > std::max(0, slow_zone) ? std::max(1, fast_step) : 1;
        step = std::min(step, distance);
        value += difference > 0 ? step : -step;
        return true;
    }

    static bool row_can_draw(int y) {
        // Protect the header/separator, but allow a last row to be partially
        // visible at the physical bottom edge. Canvas clips off-screen pixels.
        return y >= ContentClipTop && y < Canvas::Height;
    }

    static bool frame_can_draw(int y, int height) {
        return y < Canvas::Height && y + height > ContentClipTop;
    }

    Frame& current_frame() { return frames_[depth_ - 1]; }
    const Frame& current_frame() const { return frames_[depth_ - 1]; }

    bool is_glide_style() const {
        return style_.selection_style == MenuSelectionStyle::GlideFrame
            || style_.selection_style == MenuSelectionStyle::SlideFrame;
    }

    bool is_rounded_frame_style() const {
        return style_.selection_style == MenuSelectionStyle::GlideFrame
            || style_.selection_style == MenuSelectionStyle::SlideFrame
            || style_.selection_style == MenuSelectionStyle::LiquidGlass;
    }

    int frame_x() const { return static_cast<int>(style_.glass_x); }

    int frame_max_width() const {
        const int available = Canvas::Width - frame_x() - 2;
        return std::max(10, std::min(static_cast<int>(style_.glass_width), available));
    }

    int content_left_x() const {
        return std::min(Canvas::Width - 1,
                        frame_x() + static_cast<int>(style_.content_inset_left));
    }

    int content_right_edge() const {
        int edge = frame_x() + frame_max_width() - 1 - static_cast<int>(style_.content_inset_right);
        edge = std::min(edge, Canvas::Width - 1);
        return std::max(content_left_x(), edge);
    }

    std::size_t visible_row_count() const {
        return style_.visible_rows == 0 ? 1u : static_cast<std::size_t>(style_.visible_rows);
    }

    void update_viewport_for_selection(Frame& frame) {
        if (!frame.menu || frame.menu->count == 0) {
            frame.first_visible = 0;
            return;
        }

        const std::size_t rows = visible_row_count();
        const std::size_t max_first = frame.menu->count > rows
            ? frame.menu->count - rows : 0u;
        if (frame.first_visible > max_first) frame.first_visible = max_first;

        // Keep the current viewport as long as the new selection is already
        // visible. This is the key hysteresis: moving back up within the same
        // window moves only the frame instead of dragging the whole list.
        if (frame.selected < frame.first_visible) {
            frame.first_visible = frame.selected;
        } else if (frame.selected >= frame.first_visible + rows) {
            frame.first_visible = frame.selected - rows + 1u;
        }

        if (frame.first_visible > max_first) frame.first_visible = max_first;
    }

    int selected_scroll_target() const {
        return static_cast<int>(current_frame().first_visible * style_.row_height);
    }

    int effective_scroll_position() const {
        if (is_glide_style()) return glide_scroll_position_;
        return round_to_int(scroll_.position);
    }

    void sync_selection(bool snap, int direction = 0) {
        Frame& frame = current_frame();
        const std::size_t old_first_visible = frame.first_visible;
        update_viewport_for_selection(frame);
        const bool viewport_shifted = frame.first_visible != old_first_visible;

        const float selected = static_cast<float>(frame.selected * style_.row_height);
        const int glide_selected = static_cast<int>(frame.selected * style_.row_height);
        const int scroll_target = selected_scroll_target();
        const int glide_relative = glide_selected - scroll_target;

        if (snap) {
            reset_spring(selection_, selected);
            reset_spring(scroll_, static_cast<float>(scroll_target));
            glide_position_ = glide_selected;
            glide_position_target_ = glide_selected;
            glide_scroll_position_ = scroll_target;
            glide_scroll_target_ = scroll_target;
            reset_spring(glide_frame_, static_cast<float>(glide_relative));
            reset_spring(edge_handoff_, 0.0f);
            glide_accumulator_ms_ = 0;
        } else {
            selection_.target = selected;
            scroll_.target = static_cast<float>(scroll_target);
            glide_position_target_ = glide_selected;
            glide_scroll_target_ = scroll_target;
            // Do not jump glide_frame_.target to the final item. step_glide()
            // advances the virtual U8g2-style path and feeds each intermediate
            // relative position to the spring follower.

            // At an edge handoff the final on-screen row can stay unchanged
            // while the content scrolls underneath it. Give rounded frames a
            // small spring impulse so the user's key press remains visible.
            if (viewport_shifted && direction != 0 && is_rounded_frame_style()) {
                edge_handoff_.target = 0.0f;
                edge_handoff_.velocity += static_cast<float>(direction)
                    * style_.scroll_handoff_kick;
            }
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

    static void step_spring_with(Spring& spring, float dt, float stiffness, float damping_value) {
        spring.velocity += (spring.target - spring.position) * stiffness * dt;
        float damping = 1.0f - damping_value * dt;
        if (damping < 0.0f) damping = 0.0f;
        spring.velocity *= damping;
        spring.position += spring.velocity * dt;
        if (absf(spring.target - spring.position) < 0.01f && absf(spring.velocity) < 0.01f) {
            spring.position = spring.target;
            spring.velocity = 0.0f;
        }
    }

    void step_spring(Spring& spring, float dt) {
        step_spring_with(spring, dt, style_.spring_stiffness, style_.spring_damping);
    }

    void step_glide() {
        glide_toward(glide_position_, glide_position_target_,
                     static_cast<int>(style_.glide_position_fast_step),
                     static_cast<int>(style_.glide_position_slow_zone));
        glide_toward(glide_width_, glide_width_target_,
                     static_cast<int>(style_.glide_width_fast_step),
                     static_cast<int>(style_.glide_width_slow_zone));
        glide_toward(glide_scroll_position_, glide_scroll_target_,
                     static_cast<int>(style_.glide_scroll_fast_step),
                     static_cast<int>(style_.glide_scroll_slow_zone));

        // The deterministic path is only a moving target. The visible frame
        // follows it with exactly the same damped spring model as Glass.
        glide_frame_.target = static_cast<float>(glide_position_ - glide_scroll_position_);
    }

    bool glide_active() const {
        if (!is_glide_style()) return false;
        return glide_position_ != glide_position_target_
            || glide_width_ != glide_width_target_
            || glide_scroll_position_ != glide_scroll_target_;
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

        const std::uint32_t spring_elapsed = elapsed;

        // Only Glide/Slide advance the deterministic target path. Other styles
        // leave the glide spring completely isolated.
        if (is_glide_style()) {
            const std::uint32_t tick = style_.glide_tick_ms == 0 ? 1u : style_.glide_tick_ms;
            glide_accumulator_ms_ += spring_elapsed;
            while (glide_accumulator_ms_ >= tick) {
                step_glide();
                glide_accumulator_ms_ -= tick;
            }
        } else {
            glide_accumulator_ms_ = 0;
        }

        // Integrate all visible spring states. Glide/Slide's frame spring uses
        // the exact same stiffness/damping as Glass.
        while (elapsed > 0) {
            const std::uint32_t step_ms = elapsed > 8 ? 8 : elapsed;
            const float dt = static_cast<float>(step_ms) / 16.0f;
            step_spring(selection_, dt);
            step_spring(scroll_, dt);
            step_spring(panel_, dt);
            if (is_glide_style()) step_spring(glide_frame_, dt);
            if (is_rounded_frame_style()) step_spring(edge_handoff_, dt);
            elapsed -= step_ms;
        }
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
            case MenuItemKind::Choice:
                if (item.choice_index && item.choice_options && item.choice_count > 0) {
                    const std::size_t index = static_cast<std::size_t>(*item.choice_index) % item.choice_count;
                    return item.choice_options[index];
                }
                return nullptr;
            case MenuItemKind::Action:
            default:
                return nullptr;
        }
    }

    void draw_item_value(Canvas& canvas, const MenuItem& item, int y, bool selected,
                         int panel_x) const {
        char value[16]{};
        const char* right = format_item_value(item, selected, value, sizeof(value));
        if (!right) return;
        int x = content_right_edge() - canvas.text_width(right) + 1;
        if (x < content_left_x() + 30) x = content_left_x() + 30;
        canvas.text(panel_x + x, y, right);
    }

    int target_frame_width_for_selected(Canvas& canvas) const {
        if (style_.selection_style == MenuSelectionStyle::SlideFrame
            || style_.selection_style == MenuSelectionStyle::LiquidGlass) {
            return frame_max_width();
        }

        const Frame& frame = current_frame();
        if (!frame.menu || frame.menu->count == 0 || frame.selected >= frame.menu->count) {
            return frame_max_width();
        }
        if (!style_.glide_fit_content) return frame_max_width();

        const MenuItem& item = frame.menu->items[frame.selected];
        const char* label = item.label ? item.label : "?";
        int rightmost = content_left_x() + canvas.text_width(label) - 1;

        char value[16]{};
        const char* right = format_item_value(item, true, value, sizeof(value));
        if (right) rightmost = std::max(rightmost, content_right_edge());

        int width = rightmost - frame_x() + static_cast<int>(style_.content_inset_right) + 1;
        width = std::max(width, static_cast<int>(style_.glide_min_width));
        return std::min(width, frame_max_width());
    }

    void update_glide_width_target(Canvas& canvas) {
        if (!is_glide_style()) return;
        glide_width_target_ = target_frame_width_for_selected(canvas);
        if (!glide_width_initialized_) {
            glide_width_ = glide_width_target_;
            glide_width_initialized_ = true;
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

        const int scroll = effective_scroll_position();
        for (std::size_t i = 0; i < menu.count; ++i) {
            const int y = static_cast<int>(style_.content_top + i * style_.row_height) - scroll;
            if (!row_can_draw(y)) continue;
            const MenuItem& item = menu.items[i];
            canvas.text(panel_x + content_left_x(), y, item.label ? item.label : "?");
            draw_item_value(canvas, item, y, i == frame.selected, panel_x);
        }

        draw_selection(canvas, panel_x);
    }

    int shared_jelly_stretch(float relative_velocity) const {
        int stretch = round_to_int(absf(relative_velocity) * style_.glass_stretch_per_velocity);
        return std::max(0, std::min(stretch, static_cast<int>(style_.glass_max_stretch)));
    }

    void draw_jelly_frame(Canvas& canvas, int panel_x, int selection_y,
                          int base_width, float relative_velocity,
                          bool draw_motion_sheen, bool moving) const {
        const int stretch = shared_jelly_stretch(relative_velocity);
        const int max_width = frame_max_width();
        int width = std::max(10, std::min(base_width, max_width)) - stretch * 2;
        width = std::max(10, width);
        const int x = panel_x + frame_x() + stretch;
        const int base_height = std::max(7, static_cast<int>(style_.glass_height));
        const int height = base_height + stretch;
        const int y = selection_y - 2 - stretch / 2;
        if (!frame_can_draw(y, height)) return;
        const int radius = std::max(1, std::min(static_cast<int>(style_.glass_radius), height / 2));

        if (draw_motion_sheen && focused_ && moving && style_.glass_sheen_height > 0) {
            const int sheen_height = std::min(static_cast<int>(style_.glass_sheen_height),
                                              std::max(1, height - 4));
            const int sheen_y = relative_velocity >= 0.0f
                ? y + height - sheen_height - 2
                : y + 2;
            canvas.invert_rect(x + 2, sheen_y, std::max(1, width - 4), sheen_height);
        }

        canvas.round_rect(x, y, width, height, radius, true);
        if (draw_motion_sheen && focused_ && moving && width > radius * 2 + 4) {
            const int highlight_y = relative_velocity >= 0.0f ? y + 1 : y + height - 2;
            canvas.line(x + radius + 2, highlight_y,
                        x + width - radius - 3, highlight_y, true);
        }
    }

    void draw_selection(Canvas& canvas, int panel_x) {
        if (is_glide_style()) {
            const float visible_position = glide_frame_.position + edge_handoff_.position;
            const float visible_velocity = glide_frame_.velocity + edge_handoff_.velocity;
            const int selection_y = static_cast<int>(style_.content_top)
                + round_to_int(visible_position);
            const int width = style_.selection_style == MenuSelectionStyle::SlideFrame
                ? frame_max_width() : std::max(10, glide_width_);
            const bool moving = glide_active() || spring_active(glide_frame_)
                || spring_active(edge_handoff_)
                || absf(visible_velocity) > style_.glass_motion_threshold;
            draw_jelly_frame(canvas, panel_x, selection_y, width,
                             visible_velocity, false, moving);
            return;
        }

        const float base_position = selection_.position - scroll_.position;
        const float base_velocity = selection_.velocity - scroll_.velocity;
        const int selection_y = round_to_int(static_cast<float>(style_.content_top)
            + base_position + (is_rounded_frame_style() ? edge_handoff_.position : 0.0f));
        if (style_.selection_style == MenuSelectionStyle::LiquidGlass) {
            const float relative_velocity = base_velocity + edge_handoff_.velocity;
            const bool moving = spring_active(selection_) || spring_active(scroll_)
                || spring_active(edge_handoff_)
                || absf(relative_velocity) > style_.glass_motion_threshold;
            draw_jelly_frame(canvas, panel_x, selection_y, frame_max_width(),
                             relative_velocity, true, moving);
            return;
        }

        const int height = static_cast<int>(style_.indicator_height);
        if (!frame_can_draw(selection_y, height)) return;
        if (focused_) {
            canvas.fill_round_rect(panel_x + style_.indicator_x, selection_y,
                                   style_.indicator_width, style_.indicator_height, 2, true);
        } else {
            canvas.round_rect(panel_x + style_.indicator_x, selection_y,
                              style_.indicator_width, style_.indicator_height, 2, true);
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
    Spring glide_frame_{};
    Spring edge_handoff_{};
    int glide_position_{0};
    int glide_position_target_{0};
    int glide_width_{0};
    int glide_width_target_{0};
    int glide_scroll_position_{0};
    int glide_scroll_target_{0};
    std::uint32_t glide_accumulator_ms_{0};
    bool glide_width_initialized_{false};
    std::uint32_t last_draw_ms_{0};
    bool has_draw_time_{false};
};

} // namespace epui
