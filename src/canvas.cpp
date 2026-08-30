#include "epui/canvas.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace epui {
namespace {
static const std::uint8_t kFont5x7[][5] = {
{0,0,0,0,0},{0,0,0x5F,0,0},{0,7,0,7,0},{0x14,0x7F,0x14,0x7F,0x14},{0x24,0x2A,0x7F,0x2A,0x12},{0x23,0x13,8,0x64,0x62},{0x36,0x49,0x55,0x22,0x50},{0,5,3,0,0},{0,0x1C,0x22,0x41,0},{0,0x41,0x22,0x1C,0},{0x14,8,0x3E,8,0x14},{8,8,0x3E,8,8},{0,0x50,0x30,0,0},{8,8,8,8,8},{0,0x60,0x60,0,0},{0x20,0x10,8,4,2},
{0x3E,0x51,0x49,0x45,0x3E},{0,0x42,0x7F,0x40,0},{0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4B,0x31},{0x18,0x14,0x12,0x7F,0x10},{0x27,0x45,0x45,0x45,0x39},{0x3C,0x4A,0x49,0x49,0x30},{1,0x71,9,5,3},{0x36,0x49,0x49,0x49,0x36},{6,0x49,0x49,0x29,0x1E},{0,0x36,0x36,0,0},{0,0x56,0x36,0,0},{8,0x14,0x22,0x41,0},{0x14,0x14,0x14,0x14,0x14},{0,0x41,0x22,0x14,8},{2,1,0x51,9,6},
{0x32,0x49,0x79,0x41,0x3E},{0x7E,0x11,0x11,0x11,0x7E},{0x7F,0x49,0x49,0x49,0x36},{0x3E,0x41,0x41,0x41,0x22},{0x7F,0x41,0x41,0x22,0x1C},{0x7F,0x49,0x49,0x49,0x41},{0x7F,9,9,9,1},{0x3E,0x41,0x49,0x49,0x7A},{0x7F,8,8,8,0x7F},{0,0x41,0x7F,0x41,0},{0x20,0x40,0x41,0x3F,1},{0x7F,8,0x14,0x22,0x41},{0x7F,0x40,0x40,0x40,0x40},{0x7F,2,0x0C,2,0x7F},{0x7F,4,8,0x10,0x7F},{0x3E,0x41,0x41,0x41,0x3E},
{0x7F,9,9,9,6},{0x3E,0x41,0x51,0x21,0x5E},{0x7F,9,0x19,0x29,0x46},{0x46,0x49,0x49,0x49,0x31},{1,1,0x7F,1,1},{0x3F,0x40,0x40,0x40,0x3F},{0x1F,0x20,0x40,0x20,0x1F},{0x3F,0x40,0x38,0x40,0x3F},{0x63,0x14,8,0x14,0x63},{7,8,0x70,8,7},{0x61,0x51,0x49,0x45,0x43},{0,0x7F,0x41,0x41,0},{2,4,8,0x10,0x20},{0,0x41,0x41,0x7F,0},{4,2,1,2,4},{0x40,0x40,0x40,0x40,0x40},
{0,1,2,4,0},{0x20,0x54,0x54,0x54,0x78},{0x7F,0x48,0x44,0x44,0x38},{0x38,0x44,0x44,0x44,0x20},{0x38,0x44,0x44,0x48,0x7F},{0x38,0x54,0x54,0x54,0x18},{8,0x7E,9,1,2},{0x0C,0x52,0x52,0x52,0x3E},{0x7F,8,4,4,0x78},{0,0x44,0x7D,0x40,0},{0x20,0x40,0x44,0x3D,0},{0x7F,0x10,0x28,0x44,0},{0,0x41,0x7F,0x40,0},{0x7C,4,0x18,4,0x78},{0x7C,8,4,4,0x78},{0x38,0x44,0x44,0x44,0x38},
{0x7C,0x14,0x14,0x14,8},{8,0x14,0x14,0x18,0x7C},{0x7C,8,4,4,8},{0x48,0x54,0x54,0x54,0x20},{4,0x3F,0x44,0x40,0x20},{0x3C,0x40,0x40,0x20,0x7C},{0x1C,0x20,0x40,0x20,0x1C},{0x3C,0x40,0x30,0x40,0x3C},{0x44,0x28,0x10,0x28,0x44},{0x0C,0x50,0x50,0x50,0x3C},{0x44,0x64,0x54,0x4C,0x44},{0,8,0x36,0x41,0},{0,0,0x7F,0,0},{0,0x41,0x36,8,0},{8,4,8,0x10,8}
};

struct ExtraGlyph {
    std::uint32_t codepoint;
    std::uint8_t width;
    std::uint8_t columns[9];
};

static const ExtraGlyph kExtraGlyphs[] = {
    {0x00B0, 3, {0x02, 0x05, 0x02}},                         // °
    {0x00B1, 5, {0x44, 0x44, 0x5F, 0x44, 0x44}},             // ±
    {0x00B5, 5, {0x7C, 0x40, 0x40, 0x20, 0x7C}},             // µ
    {0x00D7, 5, {0x41, 0x22, 0x14, 0x22, 0x41}},             // ×
    {0x00F7, 5, {0x08, 0x08, 0x49, 0x08, 0x08}},             // ÷
    {0x03A9, 5, {0x5E, 0x61, 0x01, 0x61, 0x5E}},             // Ω
    {0x2103, 9, {0x02, 0x05, 0x02, 0x00, 0x3E, 0x41, 0x41, 0x41, 0x22}}, // ℃
    {0x2190, 5, {0x08, 0x14, 0x2A, 0x08, 0x08}},             // ←
    {0x2191, 5, {0x04, 0x02, 0x7F, 0x02, 0x04}},             // ↑
    {0x2192, 5, {0x08, 0x08, 0x2A, 0x14, 0x08}},             // →
    {0x2193, 5, {0x10, 0x20, 0x7F, 0x20, 0x10}},             // ↓
};

struct GlyphView {
    const std::uint8_t* columns;
    int width;
};

GlyphView glyph_for(std::uint32_t codepoint) {
    if (codepoint >= 0x20 && codepoint <= 0x7e) {
        return {kFont5x7[codepoint - 0x20], 5};
    }
    for (const auto& glyph : kExtraGlyphs) {
        if (glyph.codepoint == codepoint) return {glyph.columns, glyph.width};
    }
    return {kFont5x7[static_cast<unsigned char>('?') - 0x20], 5};
}

std::uint32_t next_utf8(const char*& text) {
    const auto lead = static_cast<unsigned char>(*text);
    if (lead < 0x80u) {
        ++text;
        return lead;
    }

    int continuation_count = 0;
    std::uint32_t codepoint = 0;
    std::uint32_t minimum = 0;
    if ((lead & 0xe0u) == 0xc0u) {
        continuation_count = 1;
        codepoint = lead & 0x1fu;
        minimum = 0x80u;
    } else if ((lead & 0xf0u) == 0xe0u) {
        continuation_count = 2;
        codepoint = lead & 0x0fu;
        minimum = 0x800u;
    } else if ((lead & 0xf8u) == 0xf0u) {
        continuation_count = 3;
        codepoint = lead & 0x07u;
        minimum = 0x10000u;
    } else {
        ++text;
        return '?';
    }

    const char* cursor = text + 1;
    for (int i = 0; i < continuation_count; ++i) {
        const auto byte = static_cast<unsigned char>(*cursor);
        if (byte == 0 || (byte & 0xc0u) != 0x80u) {
            ++text;
            return '?';
        }
        codepoint = (codepoint << 6u) | (byte & 0x3fu);
        ++cursor;
    }
    text = cursor;
    if (codepoint < minimum || codepoint > 0x10ffffu
        || (codepoint >= 0xd800u && codepoint <= 0xdfffu)) {
        return '?';
    }
    return codepoint;
}
}

Canvas::Canvas() { clear(); }

void Canvas::clear(bool on) {
    std::memset(buffer_, on ? 0xFF : 0x00, sizeof(buffer_));
}

void Canvas::set_origin(int x, int y) {
    origin_x_ = x;
    origin_y_ = y;
}

void Canvas::reset_origin() {
    origin_x_ = 0;
    origin_y_ = 0;
}

void Canvas::set_clip_rect(int x, int y, int w, int h) {
    if (w <= 0 || h <= 0) {
        clip_x0_ = clip_x1_ = 0;
        clip_y0_ = clip_y1_ = 0;
        return;
    }

    const int x0 = x + origin_x_;
    const int y0 = y + origin_y_;
    const int x1 = x0 + w;
    const int y1 = y0 + h;
    clip_x0_ = std::max(0, std::min(Width, x0));
    clip_y0_ = std::max(0, std::min(Height, y0));
    clip_x1_ = std::max(clip_x0_, std::max(0, std::min(Width, x1)));
    clip_y1_ = std::max(clip_y0_, std::max(0, std::min(Height, y1)));
}

void Canvas::reset_clip() {
    clip_x0_ = 0;
    clip_y0_ = 0;
    clip_x1_ = Width;
    clip_y1_ = Height;
}

void Canvas::pixel(int x, int y, bool on) {
    x += origin_x_;
    y += origin_y_;
    if (x < 0 || x >= Width || y < 0 || y >= Height) return;
    if (x < clip_x0_ || x >= clip_x1_ || y < clip_y0_ || y >= clip_y1_) return;
    const std::size_t i = static_cast<std::size_t>(x + (y / 8) * Width);
    const std::uint8_t m = static_cast<std::uint8_t>(1u << (y & 7));
    if (on) buffer_[i] |= m;
    else buffer_[i] &= static_cast<std::uint8_t>(~m);
}

void Canvas::line(int x0, int y0, int x1, int y1, bool on) {
    int dx = std::abs(x1 - x0);
    const int sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0);
    const int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    while (true) {
        pixel(x0, y0, on);
        if (x0 == x1 && y0 == y1) break;
        const int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void Canvas::rect(int x, int y, int w, int h, bool on) {
    if (w <= 0 || h <= 0) return;
    line(x, y, x + w - 1, y, on);
    line(x, y + h - 1, x + w - 1, y + h - 1, on);
    line(x, y, x, y + h - 1, on);
    line(x + w - 1, y, x + w - 1, y + h - 1, on);
}

void Canvas::fill_rect(int x, int y, int w, int h, bool on) {
    for (int yy = 0; yy < h; ++yy) line(x, y + yy, x + w - 1, y + yy, on);
}

void Canvas::round_rect(int x, int y, int w, int h, int r, bool on) {
    r = std::max(1, std::min(r, std::min(w, h) / 2));
    line(x + r, y, x + w - r - 1, y, on);
    line(x + r, y + h - 1, x + w - r - 1, y + h - 1, on);
    line(x, y + r, x, y + h - r - 1, on);
    line(x + w - 1, y + r, x + w - 1, y + h - r - 1, on);
    int px = r;
    int py = 0;
    int err = 1 - r;
    while (px >= py) {
        pixel(x + r - px, y + r - py, on);
        pixel(x + r - py, y + r - px, on);
        pixel(x + w - r - 1 + px, y + r - py, on);
        pixel(x + w - r - 1 + py, y + r - px, on);
        pixel(x + r - px, y + h - r - 1 + py, on);
        pixel(x + r - py, y + h - r - 1 + px, on);
        pixel(x + w - r - 1 + px, y + h - r - 1 + py, on);
        pixel(x + w - r - 1 + py, y + h - r - 1 + px, on);
        ++py;
        if (err < 0) err += 2 * py + 1;
        else {
            --px;
            err += 2 * (py - px) + 1;
        }
    }
}

void Canvas::fill_round_rect(int x, int y, int w, int h, int r, bool on) {
    r = std::max(1, std::min(r, std::min(w, h) / 2));
    for (int yy = 0; yy < h; ++yy) {
        int inset = 0;
        if (yy < r) {
            const int d = r - yy - 1;
            inset = std::max(0, r - static_cast<int>(std::sqrt(static_cast<double>(r * r - d * d))));
        } else if (yy >= h - r) {
            const int d = yy - (h - r);
            inset = std::max(0, r - static_cast<int>(std::sqrt(static_cast<double>(r * r - d * d))));
        }
        line(x + inset, y + yy, x + w - inset - 1, y + yy, on);
    }
}

void Canvas::circle(int cx, int cy, int r, bool on) {
    int x = r;
    int y = 0;
    int err = 0;
    while (x >= y) {
        pixel(cx + x, cy + y, on);
        pixel(cx + y, cy + x, on);
        pixel(cx - y, cy + x, on);
        pixel(cx - x, cy + y, on);
        pixel(cx - x, cy - y, on);
        pixel(cx - y, cy - x, on);
        pixel(cx + y, cy - x, on);
        pixel(cx + x, cy - y, on);
        if (err <= 0) {
            ++y;
            err += 2 * y + 1;
        }
        if (err > 0) {
            --x;
            err -= 2 * x + 1;
        }
    }
}

void Canvas::fill_circle(int cx, int cy, int r, bool on) {
    if (r < 0) return;
    for (int y = -r; y <= r; ++y) {
        const int half_width = static_cast<int>(
            std::sqrt(static_cast<double>(r * r - y * y)));
        line(cx - half_width, cy + y, cx + half_width, cy + y, on);
    }
}

void Canvas::progress_bar(int x, int y, int w, int h, float value) {
    value = std::max(0.0f, std::min(1.0f, value));
    round_rect(x, y, w, h, 2, true);
    const int inner = static_cast<int>((w - 4) * value + 0.5f);
    if (inner > 0) fill_rect(x + 2, y + 2, inner, h - 4, true);
}

void Canvas::glyph5x7(int x, int y, char c, bool on) {
    glyph5x7(x, y, static_cast<std::uint32_t>(static_cast<unsigned char>(c)), on);
}

void Canvas::glyph5x7(int x, int y, std::uint32_t codepoint, bool on) {
    const GlyphView glyph = glyph_for(codepoint);
    for (int col = 0; col < glyph.width; ++col) {
        for (int row = 0; row < 7; ++row) {
            if (glyph.columns[col] & (1u << row)) pixel(x + col, y + row, on);
        }
    }
}

int Canvas::glyph_width5x7(std::uint32_t codepoint) const {
    return glyph_for(codepoint).width;
}

void Canvas::text(int x, int y, const char* s, bool on, int spacing) {
    if (!s) return;
    while (*s) {
        const std::uint32_t codepoint = next_utf8(s);
        glyph5x7(x, y, codepoint, on);
        x += glyph_width5x7(codepoint) + spacing;
    }
}

int Canvas::text_width(const char* s, int spacing) const {
    if (!s || !*s) return 0;
    int width = 0;
    int glyphs = 0;
    while (*s) {
        width += glyph_width5x7(next_utf8(s));
        ++glyphs;
    }
    return width + (glyphs - 1) * spacing;
}

} // namespace epui
