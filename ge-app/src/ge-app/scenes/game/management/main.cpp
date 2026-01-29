#include "ge-app/scenes/game/management/main.hpp"

#include "ge-app/game/mode_indicator.hpp"
#include "ge-app/scenes/game/main.hpp"

namespace ge {
namespace scenes {
namespace game {
ManagementUIScene::ManagementUIScene(GameScene &parent)
    : ContainerScene(parent.get_app()), parent{parent}, management_menu{*this},
      status_scene{*this}, inventory_scene{*this}, map_scene{*this} {
  set_scenes(management_sub_scenes);
}

void ManagementUIScene::show_status_screen() {
  current_screen = ManagementUIScreen::Status;
}

void ManagementUIScene::show_inventory_screen() {
  current_screen = ManagementUIScreen::Inventory;
}

void ManagementUIScene::show_map_screen() {
  map_scene.on_enter();
  current_screen = ManagementUIScreen::Map;
}

void ManagementUIScene::back_to_menu() {
  current_screen = ManagementUIScreen::Menu;
}

GameMode ManagementUIScene::get_current_mode() {
  return parent.get_current_mode();
}

Inventory &ManagementUIScene::get_inventory() {
  return inventory_scene.get_inventory();
}

PlayerStats &ManagementUIScene::get_player_stats() {
  return parent.get_player_stats();
}

Clock &ManagementUIScene::get_clock() { return parent.get_clock(); }

Boat &ManagementUIScene::get_boat() {
  return parent.get_world_scene().get_boat();
}

bool ManagementUIScene::is_active() const {
  return parent.get_current_mode() == GameMode::Management;
}
} // namespace game
} // namespace scenes
} // namespace ge
