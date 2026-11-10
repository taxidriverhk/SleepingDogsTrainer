#pragma once

#include <windows.h>

#include <string>

namespace core::paths {

// Full path of a file in the game's install folder (where sdhdship.exe lives).
inline std::wstring GameDirFile(const wchar_t* fileName) {
    wchar_t exe[MAX_PATH]{};
    GetModuleFileNameW(nullptr, exe, MAX_PATH);
    std::wstring dir(exe);
    dir.resize(dir.find_last_of(L"\\/") + 1);
    return dir + fileName;
}

}  // namespace core::paths
