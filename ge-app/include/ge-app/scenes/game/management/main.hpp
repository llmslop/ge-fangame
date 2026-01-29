#pragma once

#include "ge-app/game/clock.hpp"
#include "ge-app/game/mode_indicator.hpp"
#include "ge-app/scenes/base.hpp"
#include <array>

#include "ge-app/scenes/game/management/inventory.hpp"
#include "ge-app/scenes/game/management/map.hpp"
#include "ge-app/scenes/game/management/menu.hpp"
#include "ge-app/scenes/game/management/status.hpp"

namespace ge {
namespace scenes {
class GameScene;
namespace game {

enum class ManagementUIScreen {
  Menu,
  Status,
  Inventory,
  Map,
};

class ManagementUIScene : public ContainerScene {
public:
  ManagementUIScene(GameScene &parent);

  // Navigation methods called by management menu
  void show_status_screen();
  void show_inventory_screen();
  void show_map_screen();
  void back_to_menu();

  // Getters for sub-scenes
  Inventory &get_inventory();
  PlayerStats &get_player_stats();
  Clock &get_clock();
  GameMode get_current_mode();
  Boat &get_boat();

  bool is_screen_active(ManagementUIScreen screen) const {
    return current_screen == screen;
  }

  bool is_active() const override;

private:
  GameScene &parent;
  // Management UI sub-scenes
  mgmt::MenuScene management_menu;
  mgmt::StatusScene status_scene;
  mgmt::InventoryScene inventory_scene;
  mgmt::MapScene map_scene;
  ManagementUIScreen current_screen = ManagementUIScreen::Menu;
  std::array<Scene *, 4> management_sub_scenes = {
      &management_menu, &status_scene, &inventory_scene, &map_scene};
};

} // namespace game
} // namespace scenes
} // namespace ge
