#pragma once

// Thread-safe logger writing to SDTrainer.log next to the game executable.
namespace core::log {

void Init();
void Shutdown();

void Info(const char* fmt, ...);
void Warn(const char* fmt, ...);
void Error(const char* fmt, ...);

}  // namespace core::log
