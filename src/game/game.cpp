#include "game/game.h"

#include "core/log.h"

namespace game {
namespace {

bool g_ready = false;

bool NotImplemented(const char* what) {
    core::log::Warn("game::%s is not implemented yet", what);
    return false;
}

}  // namespace

bool Init() {
    // TODO: locate the SkookumScript binding registrations in sdhdship.exe
    // (e.g. via core::pattern::Find) and store the C++ function pointers.
    g_ready = false;
    return g_ready;
}

bool IsReady() { return g_ready; }

bool SetPlayerInvulnerable(bool) { return NotImplemented("SetPlayerInvulnerable"); }
bool SetVehicleInvulnerable(bool) { return NotImplemented("SetVehicleInvulnerable"); }
bool SpawnVehicle(const std::string&) { return NotImplemented("SpawnVehicle"); }
std::vector<std::string> ListVehicles() { return {}; }

bool SetUnlimitedAmmo(bool) { return NotImplemented("SetUnlimitedAmmo"); }
bool SetNoReload(bool) { return NotImplemented("SetNoReload"); }
bool GiveWeapon(const std::string&) { return NotImplemented("GiveWeapon"); }
std::vector<std::string> ListWeapons() { return {}; }

int GetHeatLevel() { return -1; }
bool SetHeatLevel(int) { return NotImplemented("SetHeatLevel"); }
bool SetPoliceDisabled(bool) { return NotImplemented("SetPoliceDisabled"); }

bool TeleportPlayer(const Vec3&) { return NotImplemented("TeleportPlayer"); }
bool GetWaypoint(Vec3*) { return NotImplemented("GetWaypoint"); }

const std::vector<NamedLocation>& KnownLocations() {
    // Filled in once world coordinates are known.
    static const std::vector<NamedLocation> kLocations;
    return kLocations;
}

}  // namespace game
