#pragma once

// Trainer feature groups. Each group owns its settings, a menu tab and
// per-frame logic; this header is the single entry point for the core.
namespace features {

void Init();
void LoadSettings();
void SaveSettings();

// Runs every frame (render thread), whether or not the menu is open.
void Tick();
// Number of menu tabs (number keys 1..TabCount() select them).
int TabCount();
// Draws one ImGui tab per group (inside an active tab bar).
void DrawTabs();

}  // namespace features
