#pragma once

#include <windows.h>

// Subclasses the game window: toggles the menu and feeds ImGui.
// While the menu is open, input still reaches the game except the menu's own
// keys (arrows, Enter, Backspace, 1-9), mouse input over the trainer window,
// and keyboard input while typing in a text field.
namespace core::input {

void Install(HWND window);
void Uninstall();

}  // namespace core::input
