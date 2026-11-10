#pragma once

#include <cstdint>

// IDA-style signature scanning ("48 8B ?? ?? E8") over the game module's
// executable section. Used by the game layer to locate functions without
// hard-coding addresses (the exe uses ASLR).
namespace core::pattern {

// Returns the first match in sdhdship.exe's .text section, or 0.
std::uintptr_t Find(const char* signature);

// Resolves a rel32 operand (e.g. E8/E9 call target, or RIP-relative mov/lea).
// `operandOffset` is where the 4-byte displacement starts, `instrLength` the full instruction size.
std::uintptr_t ResolveRelative(std::uintptr_t instr, int operandOffset, int instrLength);

}  // namespace core::pattern
