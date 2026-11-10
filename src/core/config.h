#pragma once

// Persists trainer settings to SDTrainer.ini next to the game executable.
namespace core::config {

void Load();
void Save();

bool GetBool(const char* section, const char* key, bool fallback);
void SetBool(const char* section, const char* key, bool value);

int GetInt(const char* section, const char* key, int fallback);
void SetInt(const char* section, const char* key, int value);

}  // namespace core::config
