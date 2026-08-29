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
    void pixel(int x, int y, bool on = true);
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
};

} // namespace epui
