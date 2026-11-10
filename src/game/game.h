#pragma once

#include <string>
#include <vector>

// Thin wrappers over the game's own functions (mostly the C++ functions
// behind SkookumScript bindings such as `enable_invulnerable`,
// `set_unlimited_ammo`, `cops_set_heat_level`, `teleport_pos`).
//
// Every function is currently a stub that logs and returns false. They get
// filled in as each binding is located in sdhdship.exe.
namespace game {

struct Vec3 {
    float x, y, z;
};

struct NamedLocation {
    std::string name;
    Vec3 pos;
};

// Locates the game functions/globals. Returns false if any required one is missing.
bool Init();
// True once Init() has found everything the features need.
bool IsReady();

// Player
bool SetPlayerInvulnerable(bool enabled);

// Vehicle (the one the player is currently in)
bool SetVehicleInvulnerable(bool enabled);
bool SpawnVehicle(const std::string& vehicleId);
std::vector<std::string> ListVehicles();

// Weapons
bool SetUnlimitedAmmo(bool enabled);
bool SetNoReload(bool enabled);
bool GiveWeapon(const std::string& weaponId);
std::vector<std::string> ListWeapons();

// Police
int GetHeatLevel();  // 0..5, or -1 if unknown
bool SetHeatLevel(int level);
bool SetPoliceDisabled(bool disabled);

// Teleport
bool TeleportPlayer(const Vec3& pos);
bool GetWaypoint(Vec3* out);
const std::vector<NamedLocation>& KnownLocations();

}  // namespace game
