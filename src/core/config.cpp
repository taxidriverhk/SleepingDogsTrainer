#include "core/config.h"

#include <windows.h>

#include <map>
#include <mutex>
#include <string>

#include "core/paths.h"
#include "features/features.h"

namespace core::config {
namespace {

std::mutex g_mutex;
std::string g_path;

const std::string& Path() {
    if (g_path.empty()) {
        std::wstring w = paths::GameDirFile(L"SDTrainer.ini");
        int len = WideCharToMultiByte(CP_ACP, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
        g_path.resize(len > 0 ? len - 1 : 0);
        WideCharToMultiByte(CP_ACP, 0, w.c_str(), -1, g_path.data(), len, nullptr, nullptr);
    }
    return g_path;
}

}  // namespace

void Load() { features::LoadSettings(); }
void Save() { features::SaveSettings(); }

int GetInt(const char* section, const char* key, int fallback) {
    std::lock_guard lock(g_mutex);
    return static_cast<int>(GetPrivateProfileIntA(section, key, fallback, Path().c_str()));
}

void SetInt(const char* section, const char* key, int value) {
    std::lock_guard lock(g_mutex);
    WritePrivateProfileStringA(section, key, std::to_string(value).c_str(), Path().c_str());
}

bool GetBool(const char* section, const char* key, bool fallback) {
    return GetInt(section, key, fallback ? 1 : 0) != 0;
}

void SetBool(const char* section, const char* key, bool value) {
    SetInt(section, key, value ? 1 : 0);
}

}  // namespace core::config
