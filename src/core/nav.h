#pragma once

#include <windows.h>

#include <string>
#include <vector>

// Keyboard control for the trainer menu.
//
// Keys are captured by the window hook (any thread) and consumed once per
// frame on the render thread. Each tab draws its options through the
// widgets below; they behave like normal ImGui widgets for the mouse and add
// a keyboard highlight that Up/Down move, Left/Right adjust and Enter activates.
namespace core::nav {

// --- Key capture (called from the window hook) ---

// True for keys the menu uses while open (these are hidden from the game).
bool IsMenuKey(UINT vk);
// True for menu keys that should repeat when held (arrows).
bool IsRepeatableKey(UINT vk);
void QueueKey(UINT vk);

// --- Per frame (render thread) ---

// Drains queued keys into this frame's input. Call once before drawing.
void BeginFrame();
// Tab index requested by a number key this frame (0-based), or -1.
int TabRequest();
// True if a close key was pressed this frame.
bool CloseRequested();

// --- Focusable option list (one per tab) ---

void BeginList(int listId);
void EndList();

bool Checkbox(const char* label, bool* value);
bool Button(const char* label, bool enabled = true);
// Dropdown; Left/Right cycle the selection. Returns true if it changed.
bool Combo(const char* label, const std::vector<std::string>& items, int* selected);
// Text row with -/+ buttons; returns -1, 0 or +1 (mouse or Left/Right).
int Stepper(const char* text, bool enabled = true);

}  // namespace core::nav
