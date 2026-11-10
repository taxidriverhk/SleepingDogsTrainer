#pragma once

// Top-level trainer window. Each feature group draws its own tab.
namespace core::menu {

bool IsOpen();
void Toggle();

// Called every frame from the Present hook (ImGui frame already begun).
void Draw();

}  // namespace core::menu
