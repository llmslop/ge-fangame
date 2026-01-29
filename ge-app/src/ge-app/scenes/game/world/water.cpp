#include "ge-app/scenes/game/world/water.hpp"
#include "ge-app/scenes/game/world/main.hpp"

namespace ge {
namespace scenes {
namespace game {
namespace world {

WaterScene::WaterScene(WorldScene &parent)
    : Scene(parent.get_app()), parent(parent) {}

void WaterScene::render(Surface &fb_region) {
  auto &clock = parent.get_clock();
  auto &boat = parent.get_boat();
  water.render(&app, parent.water_region(fb_region), clock.time_in_day(app),
               boat.get_x(), boat.get_y());
}

} // namespace world
} // namespace game
} // namespace scenes
} // namespace ge
