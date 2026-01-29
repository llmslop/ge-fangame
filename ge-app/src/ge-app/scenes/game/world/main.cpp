#include "ge-app/scenes/game/world/main.hpp"

#include "ge-app/scenes/game/hud/main.hpp"
#include "ge-app/scenes/game/main.hpp"
#include "ge-app/scenes/game/world/boat.hpp"

namespace ge {
namespace scenes {
namespace game {

WorldScene::WorldScene(GameScene &parent)
    : ContainerScene(parent.get_app()), parent{parent},
      time_update_scene{*this}, sky_scene{*this}, water_scene{*this},
      dock_scene{*this}, obstacle_scene(*this), boat_scene{*this},
      fishing_scene{*this} {
  set_scenes(subscenes);
}

Inventory &WorldScene::get_inventory() { return parent.get_inventory(); }

GameMode WorldScene::get_current_mode() const {
  return parent.get_current_mode();
}

DialogScene &WorldScene::get_dialog_scene() {
  return parent.get_dialog_scene();
}

} // namespace game
} // namespace scenes
} // namespace ge
