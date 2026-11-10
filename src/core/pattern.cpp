#include "core/pattern.h"

#include <windows.h>

#include <cstring>
#include <optional>
#include <vector>

namespace core::pattern {
namespace {

std::vector<std::optional<std::uint8_t>> Parse(const char* sig) {
    std::vector<std::optional<std::uint8_t>> bytes;
    for (const char* p = sig; *p;) {
        if (*p == ' ') {
            ++p;
        } else if (*p == '?') {
            bytes.emplace_back(std::nullopt);
            while (*p == '?') ++p;
        } else {
            bytes.emplace_back(static_cast<std::uint8_t>(std::strtoul(p, const_cast<char**>(&p), 16)));
        }
    }
    return bytes;
}

bool TextSection(std::uint8_t** start, std::size_t* size) {
    auto base = reinterpret_cast<std::uint8_t*>(GetModuleHandleW(nullptr));
    auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    IMAGE_SECTION_HEADER* sec = IMAGE_FIRST_SECTION(nt);
    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++sec) {
        if (std::memcmp(sec->Name, ".text", 5) == 0) {
            *start = base + sec->VirtualAddress;
            *size = sec->Misc.VirtualSize;
            return true;
        }
    }
    return false;
}

}  // namespace

std::uintptr_t Find(const char* signature) {
    auto pat = Parse(signature);
    std::uint8_t* start = nullptr;
    std::size_t size = 0;
    if (pat.empty() || !TextSection(&start, &size) || size < pat.size()) return 0;

    for (std::size_t i = 0; i <= size - pat.size(); ++i) {
        std::size_t j = 0;
        for (; j < pat.size(); ++j) {
            if (pat[j] && start[i + j] != *pat[j]) break;
        }
        if (j == pat.size()) return reinterpret_cast<std::uintptr_t>(start + i);
    }
    return 0;
}

std::uintptr_t ResolveRelative(std::uintptr_t instr, int operandOffset, int instrLength) {
    if (!instr) return 0;
    auto rel = *reinterpret_cast<const std::int32_t*>(instr + operandOffset);
    return instr + instrLength + rel;
}

}  // namespace core::pattern
