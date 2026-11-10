#include "core/log.h"

#include <windows.h>

#include <cstdarg>
#include <cstdio>
#include <mutex>

#include "core/paths.h"

namespace core::log {
namespace {

std::mutex g_mutex;
FILE* g_file = nullptr;

void Write(const char* level, const char* fmt, va_list args) {
    std::lock_guard lock(g_mutex);
    if (!g_file) return;
    SYSTEMTIME t;
    GetLocalTime(&t);
    std::fprintf(g_file, "%02d:%02d:%02d.%03d [%s] ", t.wHour, t.wMinute, t.wSecond, t.wMilliseconds, level);
    std::vfprintf(g_file, fmt, args);
    std::fputc('\n', g_file);
    std::fflush(g_file);
}

}  // namespace

void Init() {
    std::lock_guard lock(g_mutex);
    if (g_file) return;
    _wfopen_s(&g_file, paths::GameDirFile(L"SDTrainer.log").c_str(), L"w");
}

void Shutdown() {
    std::lock_guard lock(g_mutex);
    if (g_file) {
        std::fclose(g_file);
        g_file = nullptr;
    }
}

#define SDT_LOG_IMPL(name, level)       \
    void name(const char* fmt, ...) {   \
        va_list args;                   \
        va_start(args, fmt);            \
        Write(level, fmt, args);        \
        va_end(args);                   \
    }

SDT_LOG_IMPL(Info, "INFO ")
SDT_LOG_IMPL(Warn, "WARN ")
SDT_LOG_IMPL(Error, "ERROR")

}  // namespace core::log
