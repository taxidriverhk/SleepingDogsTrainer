// Forwards the three XInput functions the game imports to the real
// System32\xinput9_1_0.dll. The real DLL is loaded lazily on first call,
// never from DllMain.
#include <windows.h>
#include <xinput.h>

#include <mutex>
#include <string>

namespace {

using GetStateFn = DWORD(WINAPI*)(DWORD, XINPUT_STATE*);
using SetStateFn = DWORD(WINAPI*)(DWORD, XINPUT_VIBRATION*);
using GetCapsFn  = DWORD(WINAPI*)(DWORD, DWORD, XINPUT_CAPABILITIES*);

GetStateFn g_getState = nullptr;
SetStateFn g_setState = nullptr;
GetCapsFn  g_getCaps  = nullptr;
std::once_flag g_loadOnce;

void LoadRealXInput() {
    std::call_once(g_loadOnce, [] {
        wchar_t sysDir[MAX_PATH]{};
        GetSystemDirectoryW(sysDir, MAX_PATH);
        std::wstring path = std::wstring(sysDir) + L"\\xinput9_1_0.dll";
        HMODULE real = LoadLibraryW(path.c_str());
        if (!real) return;
        g_getState = reinterpret_cast<GetStateFn>(GetProcAddress(real, "XInputGetState"));
        g_setState = reinterpret_cast<SetStateFn>(GetProcAddress(real, "XInputSetState"));
        g_getCaps  = reinterpret_cast<GetCapsFn>(GetProcAddress(real, "XInputGetCapabilities"));
    });
}

}  // namespace

extern "C" {

DWORD WINAPI Proxy_XInputGetState(DWORD userIndex, XINPUT_STATE* state) {
    LoadRealXInput();
    return g_getState ? g_getState(userIndex, state) : ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI Proxy_XInputSetState(DWORD userIndex, XINPUT_VIBRATION* vibration) {
    LoadRealXInput();
    return g_setState ? g_setState(userIndex, vibration) : ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI Proxy_XInputGetCapabilities(DWORD userIndex, DWORD flags, XINPUT_CAPABILITIES* caps) {
    LoadRealXInput();
    return g_getCaps ? g_getCaps(userIndex, flags, caps) : ERROR_DEVICE_NOT_CONNECTED;
}

}  // extern "C"
