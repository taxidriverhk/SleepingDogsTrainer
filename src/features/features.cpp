#include "features/features.h"

#include <imgui.h>

#include <algorithm>
#include <cstdio>
#include <iterator>
#include <string>
#include <vector>

#include "core/config.h"
#include "core/nav.h"
#include "game/game.h"

namespace features {
namespace {

// A persisted on/off option whose effect is (re-)applied through `apply`.
// Re-applying when the game layer becomes ready, or after a load screen, is
// handled by Tick() comparing `value` against `applied`.
struct Toggle {
    const char* section;
    const char* key;
    const char* label;
    bool (*apply)(bool);
    bool value = false;
    bool applied = false;
    bool saved = false;
};

Toggle g_playerGod{"Player", "Invincible", "Invincible", game::SetPlayerInvulnerable};
Toggle g_vehicleGod{"Vehicle", "Invincible", "Invincible vehicle", game::SetVehicleInvulnerable};
Toggle g_infiniteAmmo{"Weapons", "InfiniteAmmo", "Infinite ammo", game::SetUnlimitedAmmo};
Toggle g_noReload{"Weapons", "NoReload", "No reload", game::SetNoReload};
Toggle g_noPolice{"Police", "Disabled", "No police", game::SetPoliceDisabled};

Toggle* const kToggles[] = {&g_playerGod, &g_vehicleGod, &g_infiniteAmmo, &g_noReload, &g_noPolice};

int g_selectedWeapon = 0;
int g_selectedVehicle = 0;
int g_selectedLocation = 0;

// Each tab's body draws its options through core::nav so they can be driven
// by the keyboard (Up/Down select, Left/Right change, Enter activate).

void DrawPlayerTab() {
    core::nav::Checkbox(g_playerGod.label, &g_playerGod.value);
}

void DrawWeaponsTab() {
    core::nav::Checkbox(g_infiniteAmmo.label, &g_infiniteAmmo.value);
    core::nav::Checkbox(g_noReload.label, &g_noReload.value);
    ImGui::Separator();
    static const std::vector<std::string> weapons = game::ListWeapons();
    core::nav::Combo("Weapon", weapons, &g_selectedWeapon);
    if (!weapons.empty() && core::nav::Button("Give weapon")) game::GiveWeapon(weapons[g_selectedWeapon]);
}

void DrawVehicleTab() {
    core::nav::Checkbox(g_vehicleGod.label, &g_vehicleGod.value);
    ImGui::Separator();
    static const std::vector<std::string> vehicles = game::ListVehicles();
    core::nav::Combo("Vehicle", vehicles, &g_selectedVehicle);
    if (!vehicles.empty() && core::nav::Button("Spawn vehicle")) game::SpawnVehicle(vehicles[g_selectedVehicle]);
}

void DrawTeleportTab() {
    if (core::nav::Button("Teleport to waypoint")) {
        game::Vec3 wp{};
        if (game::GetWaypoint(&wp)) game::TeleportPlayer(wp);
    }
    ImGui::Separator();
    const auto& locations = game::KnownLocations();
    static std::vector<std::string> names;
    if (names.size() != locations.size()) {
        names.clear();
        for (const auto& loc : locations) names.push_back(loc.name);
    }
    core::nav::Combo("Location", names, &g_selectedLocation);
    if (!locations.empty() && core::nav::Button("Teleport")) {
        game::TeleportPlayer(locations[g_selectedLocation].pos);
    }
}

void DrawPoliceTab() {
    core::nav::Checkbox(g_noPolice.label, &g_noPolice.value);
    ImGui::Separator();
    const int heat = game::GetHeatLevel();
    char text[48];
    if (heat < 0) {
        std::snprintf(text, sizeof(text), "Heat level: unknown");
    } else {
        std::snprintf(text, sizeof(text), "Heat level: %d / 5", heat);
    }
    const bool canChange = !g_noPolice.value;
    if (int delta = core::nav::Stepper(text, canChange)) {
        const int current = heat < 0 ? 0 : heat;
        game::SetHeatLevel(std::clamp(current + delta, 0, 5));
    }
    if (core::nav::Button("Clear heat", canChange)) game::SetHeatLevel(0);
}

struct Tab {
    const char* label;  // shown with its number key, e.g. "1 Player"
    void (*draw)();
};

constexpr Tab kTabs[] = {
    {"Player", DrawPlayerTab},     {"Weapons", DrawWeaponsTab}, {"Vehicle", DrawVehicleTab},
    {"Teleport", DrawTeleportTab}, {"Police", DrawPoliceTab},
};

}  // namespace

void Init() {}

void LoadSettings() {
    for (Toggle* t : kToggles) t->saved = t->value = core::config::GetBool(t->section, t->key, false);
}

void SaveSettings() {
    for (Toggle* t : kToggles) {
        core::config::SetBool(t->section, t->key, t->value);
        t->saved = t->value;
    }
}

void Tick() {
    for (Toggle* t : kToggles) {
        if (t->value != t->saved) {
            core::config::SetBool(t->section, t->key, t->value);
            t->saved = t->value;
        }
    }
    if (!game::IsReady()) return;
    for (Toggle* t : kToggles) {
        if (t->value != t->applied && t->apply(t->value)) t->applied = t->value;
    }
}

int TabCount() { return static_cast<int>(std::size(kTabs)); }

void DrawTabs() {
    const int requested = core::nav::TabRequest();
    for (int i = 0; i < TabCount(); ++i) {
        char label[32];
        std::snprintf(label, sizeof(label), "%d %s###tab%d", i + 1, kTabs[i].label, i);
        const ImGuiTabItemFlags flags = (i == requested) ? ImGuiTabItemFlags_SetSelected : 0;
        if (ImGui::BeginTabItem(label, nullptr, flags)) {
            core::nav::BeginList(i);
            kTabs[i].draw();
            core::nav::EndList();
            ImGui::EndTabItem();
        }
    }
}

}  // namespace features
