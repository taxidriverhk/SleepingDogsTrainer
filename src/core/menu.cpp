#include "core/menu.h"

#include <imgui.h>

#include <atomic>

#include "core/nav.h"
#include "features/features.h"
#include "game/game.h"

namespace core::menu {
namespace {

std::atomic<bool> g_open{false};

}  // namespace

bool IsOpen() { return g_open; }
void Toggle() { g_open = !g_open; }

void DrawHotkeys() {
    ImGui::TextDisabled("Insert / Backspace: close    1-%d: switch tab", features::TabCount());
    ImGui::TextDisabled("Up / Down: select    Left / Right: change    Enter: toggle / activate");
    ImGui::Separator();
}

void Draw() {
    // Per-frame feature logic (e.g. re-applying toggles) runs even with the menu closed.
    features::Tick();

    nav::BeginFrame();
    if (nav::CloseRequested()) g_open = false;
    if (!g_open) return;

    ImGui::SetNextWindowSize(ImVec2(460, 480), ImGuiCond_FirstUseEver);
    bool open = true;
    if (ImGui::Begin("Sleeping Dogs Trainer", &open)) {
        DrawHotkeys();
        if (!game::IsReady()) {
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f),
                               "Game functions not mapped yet - options are placeholders.");
            ImGui::Separator();
        }
        if (ImGui::BeginTabBar("tabs")) {
            features::DrawTabs();
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
    if (!open) g_open = false;
}

}  // namespace core::menu
