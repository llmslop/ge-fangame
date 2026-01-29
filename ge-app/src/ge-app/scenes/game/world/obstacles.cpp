#include "ge-app/scenes/game/world/obstacles.hpp"
#include "ge-app/rng.hpp"
#include "ge-app/scenes/game/world/main.hpp"

namespace ge {
namespace scenes {
namespace game {
namespace world {

ObstacleScene::ObstacleScene(WorldScene &parent)
    : Scene(parent.get_app()), parent(parent) {}

void ObstacleScene::tick(float tick_dt) {
  auto &clock = parent.get_clock();
  // spawn condition: 1PM to 6PM every day
  float time_in_day = clock.time_in_day(app);
  bool can_spawn =
      time_in_day >= (13.0f / 24.0f) && time_in_day <= (18.0f / 24.0f);

  auto world_dt = parent.get_world_dt();
  auto &boat = parent.get_boat();
  auto x = boat.get_x();
  auto y = boat.get_y();

  // 1% every frame
  if (can_spawn) {
    if (rng::next_float() < 1.0 * world_dt && !whirlpools.full()) {
      // spawn a whirlpool near the boat (min dist = 100, max dist = 1000)
      float angle = rng::next_float() * 2.0f * 3.14159265f;
      float dist = 100.0f + rng::next_float() * 900.0f;
      app.log("Spawning whirlpool at distance %.1f angle %.2f radians", dist,
              angle);
      float wx = x + dist * std::cos(angle);
      float wy = y + dist * std::sin(angle);

      whirlpools.emplace_back(app, wx, wy);
      app.log("Spawned whirlpool at (%.1f, %.1f)", wx, wy);
    }
  }

  u32 total_damage = 0;
  for (usize i = 0; i < whirlpools.size();) {
    bool should_dissipate = false;
    whirlpools[i].tick(app, boat, world_dt, should_dissipate, total_damage);
    if (should_dissipate) {
      app.log("Whirlpool at (%.1f, %.1f) dissipated", whirlpools[i].get_x(),
              whirlpools[i].get_y());
      whirlpools.erase_swap(i);
      continue;
    }
    ++i;
  }

  parent.get_player_stats().apply_damage(app, total_damage);
}

void ObstacleScene::render(Surface &fb_region) {
  auto &boat = parent.get_boat();
  auto water_region = parent.water_region(fb_region);
  for (auto &whirlpool : whirlpools) {
    whirlpool.render(app, boat, water_region);
  }
}

} // namespace world
} // namespace game
} // namespace scenes
} // namespace ge
