// Entry point. DllMain only spawns the init thread; all real work happens there.
#include <windows.h>

#include "core/config.h"
#include "core/log.h"
#include "core/render_hook.h"
#include "features/features.h"
#include "game/game.h"

namespace {

bool IsGameProcess() {
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    const wchar_t* name = wcsrchr(path, L'\\');
    name = name ? name + 1 : path;
    return _wcsicmp(name, L"sdhdship.exe") == 0;
}

DWORD WINAPI InitThread(LPVOID) {
    core::log::Init();
    core::log::Info("SDTrainer starting");

    core::config::Load();
    features::Init();

    if (!game::Init()) {
        core::log::Warn("Game layer init incomplete; game features will be unavailable");
    }
    if (!core::render::Install()) {
        core::log::Error("Render hook install failed; menu will not be shown");
        return 1;
    }
    core::log::Info("SDTrainer ready (menu key: Insert)");
    return 0;
}

}  // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(module);
        // The proxy still forwards XInput in any process; the trainer only runs in the game.
        if (IsGameProcess()) {
            if (HANDLE t = CreateThread(nullptr, 0, InitThread, nullptr, 0, nullptr)) CloseHandle(t);
        }
        break;
    case DLL_PROCESS_DETACH:
        // Settings are saved as they change (features::Tick), not here under the loader lock.
        core::log::Shutdown();
        break;
    }
    return TRUE;
}
