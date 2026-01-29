#pragma once

#include "ge-app/game/clock.hpp"
#include "ge-app/game/inventory.hpp"
#include "ge-app/game/player_stats.hpp"
#include "ge-app/scenes/base.hpp"
#include "ge-app/scenes/game/bgm.hpp"
#include "ge-app/scenes/game/hud/main.hpp"
#include "ge-app/scenes/game/management/main.hpp"
#include "ge-app/scenes/game/world/main.hpp"
#include <array>

namespace ge {
namespace scenes {
class RootScene;

class GameScene : public ContainerScene {
public:
  explicit GameScene(RootScene &parent);

  game::WorldScene &get_world_scene() { return world; }
  DialogScene &get_dialog_scene();

  Clock &get_clock() { return world.get_clock(); }
  Inventory &get_inventory() { return management_ui.get_inventory(); }
  PlayerStats &get_player_stats() { return world.get_player_stats(); }
  GameMode get_current_mode() { return hud.get_current_mode(); }

  void on_mode_changed(GameMode old_mode, GameMode new_mode);

  bool is_active() const override;

  void on_enter() { bgm.on_enter(); }
  void start_new_game() {
    world.start_new_game();
    management_ui.back_to_menu();
  }
  void on_exit() { bgm.on_exit(); }

private:
  RootScene &parent;

  // Sub-scenes
  game::BGMScene bgm;
  game::WorldScene world;
  game::HUDScene hud;
  game::ManagementUIScene management_ui;
  std::array<Scene *, 4> sub_scene_array = {&bgm, &world, &hud, &management_ui};

  // Tutorial system
  // TutorialSystem tutorial;
  // GameModeIndicator mode_indicator;

  // i64 last_frame_world_time;
  // bool is_accelerating;
};

} // namespace scenes
} // namespace ge
