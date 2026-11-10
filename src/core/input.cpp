#include "core/input.h"

#include <imgui.h>
#include <imgui_impl_win32.h>

#include "core/log.h"
#include "core/menu.h"
#include "core/nav.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace core::input {
namespace {

constexpr UINT kMenuKey = VK_INSERT;  // F5/F6 are taken by the DLSS add-on

HWND g_window = nullptr;
WNDPROC g_origWndProc = nullptr;

enum class Device { None, Mouse, Keyboard };

// Which device a window message comes from. WM_INPUT (raw input, which the
// game uses for keyboard and mouse) is classified by its raw-input header.
Device MessageDevice(UINT msg, LPARAM lParam) {
    if (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST) return Device::Mouse;
    if (msg >= WM_KEYFIRST && msg <= WM_KEYLAST) return Device::Keyboard;
    if (msg == WM_INPUT) {
        RAWINPUTHEADER header{};
        UINT size = sizeof(header);
        if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_HEADER, &header, &size,
                            sizeof(RAWINPUTHEADER)) == sizeof(header)) {
            if (header.dwType == RIM_TYPEMOUSE) return Device::Mouse;
            if (header.dwType == RIM_TYPEKEYBOARD) return Device::Keyboard;
        }
    }
    return Device::None;
}

// Only block input the menu is actually using, so the game stays playable
// while the menu is open: mouse over the trainer window, or typing in a text field.
bool MenuWantsInput(Device device) {
    if (!ImGui::GetCurrentContext()) return false;
    const ImGuiIO& io = ImGui::GetIO();
    switch (device) {
    case Device::Mouse: return io.WantCaptureMouse;
    case Device::Keyboard: return io.WantTextInput;
    default: return false;
    }
}

// Handles the menu's own keys (arrows, Enter, Backspace, 1-9) while it is open.
// Returns true if the message must be hidden from the game. Key presses are
// queued from WM_KEYDOWN; the matching WM_CHAR and raw-input key-down events
// are swallowed too so the game never acts on them. Key-ups pass through,
// which is harmless and avoids keys getting stuck in the game.
bool HandleMenuKey(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN: {
        const UINT vk = static_cast<UINT>(wParam);
        if (!nav::IsMenuKey(vk)) return false;
        const bool isRepeat = (lParam & (1 << 30)) != 0;
        if (!isRepeat || nav::IsRepeatableKey(vk)) nav::QueueKey(vk);
        return true;
    }
    case WM_CHAR:
        return (wParam >= '1' && wParam <= '9') || wParam == '\r' || wParam == '\b';
    case WM_INPUT: {
        RAWINPUT raw{};
        UINT size = sizeof(raw);
        if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &raw, &size,
                            sizeof(RAWINPUTHEADER)) == static_cast<UINT>(-1)) {
            return false;
        }
        return raw.header.dwType == RIM_TYPEKEYBOARD && !(raw.data.keyboard.Flags & RI_KEY_BREAK) &&
               nav::IsMenuKey(raw.data.keyboard.VKey);
    }
    default:
        return false;
    }
}

LRESULT CALLBACK HookWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_KEYDOWN && wParam == kMenuKey && !(lParam & (1 << 30))) {
        menu::Toggle();
        return 0;
    }

    if (menu::IsOpen()) {
        ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);
        const bool typingInMenu = ImGui::GetCurrentContext() && ImGui::GetIO().WantTextInput;
        if ((!typingInMenu && HandleMenuKey(msg, wParam, lParam)) ||
            MenuWantsInput(MessageDevice(msg, lParam))) {
            // WM_INPUT must still reach DefWindowProc so Windows frees the raw-input data.
            return msg == WM_INPUT ? DefWindowProcW(hwnd, msg, wParam, lParam) : 0;
        }
    }
    return CallWindowProcW(g_origWndProc, hwnd, msg, wParam, lParam);
}

}  // namespace

void Install(HWND window) {
    if (g_origWndProc || !window) return;
    g_window = window;
    g_origWndProc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&HookWndProc)));
    log::Info("WndProc hooked (original %p)", g_origWndProc);
}

void Uninstall() {
    if (g_origWndProc && g_window) {
        SetWindowLongPtrW(g_window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_origWndProc));
    }
    g_origWndProc = nullptr;
    g_window = nullptr;
}

}  // namespace core::input
