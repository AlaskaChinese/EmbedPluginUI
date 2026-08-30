#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <chrono>
#include <cstdint>
#include <limits>
#include "app.hpp"
#include "epui/input_plugin.hpp"

using namespace epui;

namespace {
constexpr int kScale = 7;
constexpr int kPad = 24;
constexpr int kClientW = Canvas::Width * kScale + kPad * 2;
constexpr int kClientH = Canvas::Height * kScale + kPad * 2;

using Clock = std::chrono::steady_clock;
epui::demo::SimulatorUi g_app;

std::uint32_t now_ms() {
    return static_cast<std::uint32_t>(GetTickCount64() & 0xffffffffu);
}

std::uint32_t elapsed_us(Clock::time_point begin, Clock::time_point end) {
    using namespace std::chrono;
    const auto value = duration_cast<microseconds>(end - begin).count();
    if (value <= 0) return 0;
    const auto limit = static_cast<long long>(std::numeric_limits<std::uint32_t>::max());
    return static_cast<std::uint32_t>(value > limit ? limit : value);
}

bool process_memory_probe(void*, DebugMemoryStats& out) {
    PROCESS_MEMORY_COUNTERS counters{};
    counters.cb = sizeof(counters);
    if (!GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters))) return false;
    out.used_bytes = static_cast<std::uint64_t>(counters.WorkingSetSize);
    out.total_bytes = 0;
    return true;
}

void handle_key(WPARAM key) {
    InputEvent event{};
    switch (key) {
    case VK_RIGHT: event.key = Key::Next; break;
    case VK_LEFT: event.key = Key::Prev; break;
    case VK_DOWN:
        if ((GetKeyState(VK_CONTROL) & 0x8000) == 0) return;
        event.key = Key::ScrollDown;
        break;
    case VK_UP:
        if ((GetKeyState(VK_CONTROL) & 0x8000) == 0) return;
        event.key = Key::ScrollUp;
        break;
    case VK_RETURN: event.key = Key::Select; break;
    case VK_ESCAPE: event.key = Key::Back; break;
    default: return;
    }
    g_app.ui().handle(event, now_ms());
}

void handle_char(WPARAM value) {
    if (value == 0 || value > 0x7e) return;
    const char ch = static_cast<char>(value);
    if (ch == '\r' || ch == 0x1b) return;
    InputEvent event{};
    event.ch = ch;
    g_app.ui().handle(event, now_ms());
}

void paint(HWND hwnd) {
    PAINTSTRUCT ps{};
    HDC hdc = BeginPaint(hwnd, &ps);
    RECT client{};
    GetClientRect(hwnd, &client);
    HDC mem = CreateCompatibleDC(hdc);
    HBITMAP bitmap = CreateCompatibleBitmap(hdc, client.right, client.bottom);
    HGDIOBJ old = SelectObject(mem, bitmap);
    HBRUSH bg = CreateSolidBrush(RGB(19, 21, 24));
    FillRect(mem, &client, bg);
    DeleteObject(bg);

    const auto render_begin = Clock::now();
    g_app.ui().render(g_app.canvas(), now_ms());
    g_app.diagnostics().record_render_time_us(elapsed_us(render_begin, Clock::now()));
    const auto transfer_begin = Clock::now();

    RECT bezel{kPad - 8, kPad - 8,
               kPad + Canvas::Width * kScale + 8,
               kPad + Canvas::Height * kScale + 8};
    HBRUSH bb = CreateSolidBrush(RGB(6, 7, 8));
    FillRect(mem, &bezel, bb);
    DeleteObject(bb);

    HBRUSH pixel = CreateSolidBrush(RGB(229, 242, 255));
    const std::uint8_t* fb = g_app.canvas().data();
    for (int y = 0; y < Canvas::Height; ++y) {
        int x = 0;
        while (x < Canvas::Width) {
            const std::size_t i = static_cast<std::size_t>(x + (y / 8) * Canvas::Width);
            if (((fb[i] >> (y & 7)) & 1u) == 0u) {
                ++x;
                continue;
            }
            const int begin = x;
            do {
                ++x;
                if (x >= Canvas::Width) break;
                const std::size_t next = static_cast<std::size_t>(
                    x + (y / 8) * Canvas::Width);
                if (((fb[next] >> (y & 7)) & 1u) == 0u) break;
            } while (true);
            RECT r{kPad + begin * kScale, kPad + y * kScale,
                   kPad + x * kScale, kPad + (y + 1) * kScale};
            FillRect(mem, &r, pixel);
        }
    }
    DeleteObject(pixel);

    SetBkMode(mem, TRANSPARENT);
    SetTextColor(mem, RGB(165, 173, 184));
    const char* hint = "Left/Right: move/edit   Ctrl+Up/Down: output page   Enter: run";
    TextOutA(mem, kPad, kClientH - 16, hint, lstrlenA(hint));
    BitBlt(hdc, 0, 0, client.right, client.bottom, mem, 0, 0, SRCCOPY);
    g_app.diagnostics().record_transfer(elapsed_us(transfer_begin, Clock::now()),
                                        Canvas::BufferSize, true);

    SelectObject(mem, old);
    DeleteObject(bitmap);
    DeleteDC(mem);
    EndPaint(hwnd, &ps);
}

LRESULT CALLBACK proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: SetTimer(hwnd, 1, 16, nullptr); return 0;
    case WM_TIMER: InvalidateRect(hwnd, nullptr, FALSE); return 0;
    case WM_KEYDOWN: handle_key(wp); InvalidateRect(hwnd, nullptr, FALSE); return 0;
    case WM_CHAR: handle_char(wp); InvalidateRect(hwnd, nullptr, FALSE); return 0;
    case WM_PAINT: paint(hwnd); return 0;
    case WM_DESTROY: KillTimer(hwnd, 1); PostQuitMessage(0); return 0;
    default: return DefWindowProc(hwnd, msg, wp, lp);
    }
}
} // namespace

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int show) {
    g_app.diagnostics().set_memory_probe(process_memory_probe);

    const char* klass = "EmbedPluginUISimulator";
    WNDCLASSA wc{};
    wc.lpfnWndProc = proc;
    wc.hInstance = instance;
    wc.lpszClassName = klass;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    if (!RegisterClassA(&wc)) return 1;

    RECT r{0, 0, kClientW, kClientH};
    AdjustWindowRect(&r, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE);
    HWND hwnd=CreateWindowExA(0, klass, "EmbedPluginUI - 128x64 Simulator",
                              WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                              CW_USEDEFAULT, CW_USEDEFAULT,
                              r.right - r.left, r.bottom - r.top,
                              nullptr, nullptr, instance, nullptr);
    if (!hwnd) return 2;

    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);
    MSG msg{};
    while (GetMessage(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return static_cast<int>(msg.wParam);
}
