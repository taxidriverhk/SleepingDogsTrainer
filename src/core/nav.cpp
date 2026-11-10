#include "core/nav.h"

#include <imgui.h>

#include <map>
#include <mutex>

namespace core::nav {
namespace {

// --- Key queue (window thread -> render thread) ---
std::mutex g_queueMutex;
std::vector<UINT> g_queue;

// --- This frame's input ---
struct FrameInput {
    int moveY = 0;  // net Up/Down presses (+ = down)
    int moveX = 0;  // net Left/Right presses (+ = right)
    bool activate = false;
    bool close = false;
    int tab = -1;
};
FrameInput g_in;

// --- Current list state ---
struct ListState {
    int focused = 0;
    int count = 0;  // number of focusable items drawn last frame
};
std::map<int, ListState> g_lists;
ListState* g_list = nullptr;
int g_index = 0;           // index of the next item in the current list
bool g_focusMoved = false;  // focus changed by keyboard this frame

constexpr ImU32 kHighlightFill = IM_COL32(66, 150, 250, 50);
constexpr ImU32 kHighlightBar = IM_COL32(66, 150, 250, 230);

// Claims the next item slot. Returns true if it has keyboard focus.
bool NextItem() { return g_list && g_index++ == g_list->focused; }

// Draws the highlight over the item just submitted and handles mouse focus.
void FinishItem(bool focused, ImVec2 rowStart) {
    const int index = g_index - 1;
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && g_list) {
        g_list->focused = index;
    }
    if (!focused) return;

    ImDrawList* draw = ImGui::GetWindowDrawList();
    // After an item the cursor sits at the start of the next line, so this is the row's right edge.
    const ImVec2 rowEnd(ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x,
                        ImGui::GetItemRectMax().y);
    const ImVec2 start(rowStart.x - 4.0f, rowStart.y - 1.0f);
    draw->AddRectFilled(start, ImVec2(rowEnd.x, rowEnd.y + 1.0f), kHighlightFill, 3.0f);
    draw->AddRectFilled(start, ImVec2(start.x + 3.0f, rowEnd.y + 1.0f), kHighlightBar);
    if (g_focusMoved && !ImGui::IsItemVisible()) ImGui::SetScrollHereY(0.5f);
}

}  // namespace

bool IsMenuKey(UINT vk) {
    switch (vk) {
    case VK_UP: case VK_DOWN: case VK_LEFT: case VK_RIGHT:
    case VK_RETURN: case VK_BACK:
        return true;
    default:
        return vk >= '1' && vk <= '9';
    }
}

bool IsRepeatableKey(UINT vk) {
    return vk == VK_UP || vk == VK_DOWN || vk == VK_LEFT || vk == VK_RIGHT;
}

void QueueKey(UINT vk) {
    std::lock_guard lock(g_queueMutex);
    if (g_queue.size() < 64) g_queue.push_back(vk);
}

void BeginFrame() {
    std::vector<UINT> keys;
    {
        std::lock_guard lock(g_queueMutex);
        keys.swap(g_queue);
    }
    g_in = {};
    for (UINT vk : keys) {
        switch (vk) {
        case VK_UP: --g_in.moveY; break;
        case VK_DOWN: ++g_in.moveY; break;
        case VK_LEFT: --g_in.moveX; break;
        case VK_RIGHT: ++g_in.moveX; break;
        case VK_RETURN: g_in.activate = true; break;
        case VK_BACK: g_in.close = true; break;
        default:
            if (vk >= '1' && vk <= '9') g_in.tab = static_cast<int>(vk - '1');
        }
    }
}

int TabRequest() { return g_in.tab; }
bool CloseRequested() { return g_in.close; }

void BeginList(int listId) {
    g_list = &g_lists[listId];
    g_index = 0;
    g_focusMoved = false;
    if (g_in.moveY != 0 && g_list->count > 0) {
        const int n = g_list->count;
        g_list->focused = ((g_list->focused + g_in.moveY) % n + n) % n;  // wraps around
        g_focusMoved = true;
    }
}

void EndList() {
    if (!g_list) return;
    g_list->count = g_index;
    if (g_list->focused >= g_index) g_list->focused = g_index > 0 ? g_index - 1 : 0;
    g_list = nullptr;
}

bool Checkbox(const char* label, bool* value) {
    const bool focused = NextItem();
    const ImVec2 start = ImGui::GetCursorScreenPos();
    bool changed = ImGui::Checkbox(label, value);
    if (focused && g_in.activate) {
        *value = !*value;
        changed = true;
    }
    FinishItem(focused, start);
    return changed;
}

bool Button(const char* label, bool enabled) {
    const bool focused = NextItem();
    const ImVec2 start = ImGui::GetCursorScreenPos();
    ImGui::BeginDisabled(!enabled);
    bool pressed = ImGui::Button(label);
    ImGui::EndDisabled();
    if (focused && enabled && g_in.activate) pressed = true;
    FinishItem(focused, start);
    return pressed;
}

bool Combo(const char* label, const std::vector<std::string>& items, int* selected) {
    if (items.empty()) {
        ImGui::TextDisabled("%s: list not available yet", label);
        return false;
    }
    const int n = static_cast<int>(items.size());
    if (*selected < 0 || *selected >= n) *selected = 0;

    const bool focused = NextItem();
    const ImVec2 start = ImGui::GetCursorScreenPos();
    bool changed = false;
    if (ImGui::BeginCombo(label, items[*selected].c_str())) {
        for (int i = 0; i < n; ++i) {
            if (ImGui::Selectable(items[i].c_str(), i == *selected)) {
                *selected = i;
                changed = true;
            }
        }
        ImGui::EndCombo();
    }
    if (focused && g_in.moveX != 0) {
        *selected = ((*selected + g_in.moveX) % n + n) % n;
        changed = true;
    }
    FinishItem(focused, start);
    return changed;
}

int Stepper(const char* text, bool enabled) {
    const bool focused = NextItem();
    const ImVec2 start = ImGui::GetCursorScreenPos();
    int delta = 0;
    ImGui::PushID(text);
    ImGui::BeginDisabled(!enabled);
    if (ImGui::Button("-")) delta = -1;
    ImGui::SameLine();
    if (ImGui::Button("+")) delta = +1;
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::TextUnformatted(text);
    ImGui::PopID();
    if (focused && enabled && g_in.moveX != 0) delta = g_in.moveX < 0 ? -1 : +1;
    FinishItem(focused, start);
    return delta;
}

}  // namespace core::nav
