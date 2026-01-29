#include "ge-app/scenes/game/management/map.hpp"
#include "ge-app/scenes/game/management/main.hpp"

namespace ge {
namespace scenes {
namespace game {
namespace mgmt {

MapScene::MapScene(ManagementUIScene &parent)
    : Scene{parent.get_app()}, parent{parent} {
  // Initialize to boat's current position
  map_offset_x = static_cast<float>(get_boat_x());
  map_offset_y = static_cast<float>(get_boat_y());

  // Initialize bookmarks
  for (u32 i = 0; i < MAX_BOOKMARKS; i++) {
    bookmarks[i].active = false;
  }
}

void MapScene::on_back_action() { parent.back_to_menu(); }

bool MapScene::is_active() const {
  return parent.is_screen_active(ManagementUIScreen::Map);
}

i32 MapScene::get_boat_x() { return parent.get_boat().get_x(); }

i32 MapScene::get_boat_y() { return parent.get_boat().get_y(); }

Clock &MapScene::get_clock() { return parent.get_clock(); }

} // namespace mgmt
} // namespace game
} // namespace scenes
} // namespace ge
