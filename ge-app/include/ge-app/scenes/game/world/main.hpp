#pragma once

#include <array>

#include "ge-app/game/boat.hpp"
#include "ge-app/game/clock.hpp"
#include "ge-app/game/inventory.hpp"
#include "ge-app/game/player_stats.hpp"
#include "ge-app/scenes/base.hpp"
#include "ge-app/scenes/game/hud/main.hpp"
#include "ge-app/scenes/game/world/boat.hpp"
#include "ge-app/scenes/game/world/dock.hpp"
#include "ge-app/scenes/game/world/fishing.hpp"
#include "ge-app/scenes/game/world/obstacles.hpp"
#include "ge-app/scenes/game/world/sky.hpp"
#include "ge-app/scenes/game/world/time_update.hpp"
#include "ge-app/scenes/game/world/water.hpp"

namespace ge {
namespace scenes {
class GameScene;
namespace game {

namespace world {
class TimeUpdateScene;
}; // namespace world

class WorldScene;

class WorldDtSetter {
  f32 &world_dt_ptr;

  WorldDtSetter(f32 &world_dt) : world_dt_ptr(world_dt) {}

  void set(f32 dt) { world_dt_ptr = dt; }

  friend class WorldScene;
  friend class world::TimeUpdateScene;
};

class WorldScene : public ContainerScene {
public:
  WorldScene(GameScene &parent);

  void start_new_game() { time_update_scene.start_new_game(); }

  Clock &get_clock() { return time_update_scene.get_clock(); }
  PlayerStats &get_player_stats() {
    return time_update_scene.get_player_stats();
  }
  WorldDtSetter get_world_dt_setter() { return WorldDtSetter(world_dt); }
  f32 get_world_dt() const { return world_dt; }

  Boat &get_boat() { return boat_scene.get_boat(); }
  Inventory &get_inventory();

  template <class ElemT>
  BaseSurface<ElemT> sky_region(BaseSurface<ElemT> &fb_region) {
    return fb_region.subsurface(0, 0, App::WIDTH, SKY_HEIGHT);
  }

  template <class ElemT>
  BaseSurface<ElemT> water_region(BaseSurface<ElemT> &fb_region) {
    return fb_region.subsurface(0, SKY_HEIGHT, App::WIDTH,
                                App::HEIGHT - SKY_HEIGHT);
  }

  GameMode get_current_mode() const;
  DialogScene &get_dialog_scene();

  void on_mode_changed(GameMode old_mode, GameMode new_mode) {
    if (old_mode == new_mode)
      return;
    boat_scene.on_mode_changed(old_mode, new_mode);
    fishing_scene.on_mode_changed(old_mode, new_mode);
  }

private:
  static constexpr u32 SKY_HEIGHT = 80;
  GameScene &parent;

  // Temporary storage
  f32 world_dt = 0.0; // world delta time

  // Scenes
  world::TimeUpdateScene time_update_scene;
  world::SkyScene sky_scene;
  world::WaterScene water_scene;
  world::DockScene dock_scene;
  world::ObstacleScene obstacle_scene;
  world::BoatScene boat_scene;
  world::FishingScene fishing_scene;
  std::array<Scene *, 7> subscenes = {
      &time_update_scene, &sky_scene,  &water_scene,  &dock_scene,
      &obstacle_scene,    &boat_scene, &fishing_scene};
};

} // namespace game
} // namespace scenes
} // namespace ge
