#pragma once

#include <cstddef>
#include <cstdint>

namespace epui {

class Canvas {
public:
    static constexpr int Width = 128;
    static constexpr int Height = 64;
    static constexpr std::size_t BufferSize = Width * Height / 8;

    Canvas();
    void clear(bool on = false);
    void set_origin(int x, int y);
    void reset_origin();

    // Clip coordinates use the current local coordinate system and are
    // converted to framebuffer coordinates when the rectangle is set.
    void set_clip_rect(int x, int y, int w, int h);
    void reset_clip();

    void pixel(int x, int y, bool on = true);
    void invert_pixel(int x, int y) {
        x += origin_x_;
        y += origin_y_;
        if (x < 0 || x >= Width || y < 0 || y >= Height) return;
        if (x < clip_x0_ || x >= clip_x1_ || y < clip_y0_ || y >= clip_y1_) return;
        const std::size_t i = static_cast<std::size_t>(x + (y / 8) * Width);
        const std::uint8_t mask = static_cast<std::uint8_t>(1u << (y & 7));
        buffer_[i] ^= mask;
    }
    void invert_rect(int x, int y, int w, int h) {
        if (w <= 0 || h <= 0) return;
        for (int yy = 0; yy < h; ++yy) {
            for (int xx = 0; xx < w; ++xx) invert_pixel(x + xx, y + yy);
        }
    }
    void line(int x0, int y0, int x1, int y1, bool on = true);
    void rect(int x, int y, int w, int h, bool on = true);
    void fill_rect(int x, int y, int w, int h, bool on = true);
    void round_rect(int x, int y, int w, int h, int r, bool on = true);
    void fill_round_rect(int x, int y, int w, int h, int r, bool on = true);
    void circle(int cx, int cy, int r, bool on = true);
    void progress_bar(int x, int y, int w, int h, float value);
    void glyph5x7(int x, int y, char c, bool on = true);
    void text(int x, int y, const char* s, bool on = true, int spacing = 1);
    int text_width(const char* s, int spacing = 1) const;
    const std::uint8_t* data() const { return buffer_; }
    std::uint8_t* data() { return buffer_; }

private:
    std::uint8_t buffer_[BufferSize];
    int origin_x_{0};
    int origin_y_{0};
    int clip_x0_{0};
    int clip_y0_{0};
    int clip_x1_{Width};
    int clip_y1_{Height};
};

} // namespace epui
