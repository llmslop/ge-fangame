#include "ge-app/scenes/game/world/time_update.hpp"
#include "ge-app/scenes/game/world/main.hpp"

namespace ge {
namespace scenes {
namespace game {
namespace world {

TimeUpdateScene::TimeUpdateScene(WorldScene &parent)
    : Scene(parent.get_app()), parent(parent) {}

void TimeUpdateScene::tick(float dt) {
  auto now = parent.get_clock().get_game_timer().get(app);
  if (last_frame_world_time >= 0) {
    parent.get_world_dt_setter().set((now - last_frame_world_time) * 1e-3f);
  }
  last_frame_world_time = now;

  player_stats.update(parent.get_world_dt(), parent.get_boat().get_angle(),
                      parent.get_boat().get_y(),
                      parent.get_current_mode() == GameMode::Steering);
  clock.update_multiplier(app, parent.get_current_mode());
}

bool TimeUpdateScene::on_button_held(Button btn) {
  clock.begin_sped_up();
  return true;
}
bool TimeUpdateScene::on_button_finished_hold(Button btn) {
  clock.end_sped_up();
  return true;
}
} // namespace world
} // namespace game
} // namespace scenes
} // namespace ge
