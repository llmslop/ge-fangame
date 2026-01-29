#include "ge-app/scenes/game/world/dock.hpp"
#include "ge-app/scenes/game/world/main.hpp"

namespace ge {
namespace scenes {
namespace game {
namespace world {

DockScene::DockScene(WorldScene &parent)
    : Scene(parent.get_app()), parent(parent) {}

void DockScene::render(Surface &fb_region) {
  auto &boat = parent.get_boat();
  auto &clock = parent.get_clock();
  dock.render(app, fb_region, clock, boat.get_x(), boat.get_y());
}

} // namespace world
} // namespace game
} // namespace scenes
} // namespace ge
