#pragma once

#include "ge-app/game/clock.hpp"
#include "ge-app/game/player_stats.hpp"
#include "ge-app/scenes/base.hpp"

namespace ge {
namespace scenes {
namespace game {
class WorldScene;
namespace world {
class TimeUpdateScene : public Scene {
public:
  TimeUpdateScene(WorldScene &parent);

  void tick(float dt) override;

  bool on_button_held(Button btn) override;
  bool on_button_finished_hold(Button btn) override;

  // void on_exit(); -- not necessary as is_active() is always true

  Clock &get_clock() { return clock; }
  PlayerStats &get_player_stats() { return player_stats; }

  void start_new_game() {
    last_frame_world_time = -1;
    clock.reset();
    player_stats = PlayerStats();
  }

private:
  WorldScene &parent;

  i64 last_frame_world_time = -1;

  Clock clock;
  PlayerStats player_stats;
};
} // namespace world
} // namespace game
} // namespace scenes
} // namespace ge
