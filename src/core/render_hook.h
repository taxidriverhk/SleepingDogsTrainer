#pragma once

// Hooks IDXGISwapChain::Present / ResizeBuffers to draw the ImGui menu.
// Works whether or not ReShade (or another dxgi wrapper) is installed: the
// vtable is taken from a dummy swap chain created through whatever dxgi.dll
// the game loaded, which is the same class the game's own swap chain uses.
namespace core::render {

bool Install();
void Uninstall();

}  // namespace core::render
