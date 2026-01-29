#pragma once

#include "assets/out/textures/whirlpool.h"
#include "ge-app/aabb.hpp"
#include "ge-app/arrayvec.hpp"
#include "ge-app/game/boat.hpp"
#include "ge-app/rng.hpp"
#include "ge-app/scenes/base.hpp"
#include <cmath>

namespace ge {
namespace scenes {
namespace game {
class WorldScene;
namespace world {

class Whirlpool {
public:
  Whirlpool(App &app, float x, float y)
      : x(x), y(y), spawn_time(app.now() * 1e-3) {}

  float get_x() const { return x; }
  float get_y() const { return y; }

  AABB hitbox() const {
    AABB texture_hitbox{
        static_cast<i32>(x - whirlpool_FRAME_WIDTH / 2),
        static_cast<i32>(y - whirlpool_FRAME_HEIGHT / 2),
        whirlpool_FRAME_WIDTH,
        whirlpool_FRAME_HEIGHT,
    };

    return texture_hitbox;
  }

  void tick(App &app, Boat &boat, float dt, bool &should_dissipate,
            u32 &damage_reduction) {
    auto boat_x = boat.get_x();
    auto boat_y = boat.get_y();
    // --- movement ---
    float to_boat_x = static_cast<float>(boat_x) - x;
    float to_boat_y = static_cast<float>(boat_y) - y;

    float len = std::sqrt(to_boat_x * to_boat_x + to_boat_y * to_boat_y);
    if (len > 0.001f) {
      to_boat_x /= len;
      to_boat_y /= len;
    }

    auto rand_unit = []() {
      return (rng::next_float() * 2.0f) - 1.0f; // -1.0 to 1.0
    };

    // Random drift
    float rand_x = rand_unit();
    float rand_y = rand_unit();

    // Bias weights (tweakable)
    constexpr float BOAT_BIAS = 0.6f; // attraction strength
    constexpr float RAND_BIAS = 1.0f; // chaos strength

    float dir_x = to_boat_x * BOAT_BIAS + rand_x * RAND_BIAS;
    float dir_y = to_boat_y * BOAT_BIAS + rand_y * RAND_BIAS;

    // Normalize final direction
    float dir_len = std::sqrt(dir_x * dir_x + dir_y * dir_y);
    if (dir_len > 0.001f) {
      dir_x /= dir_len;
      dir_y /= dir_len;

      float speed = 12.0f; // faster than before
      x += dir_x * speed * dt;
      y += dir_y * speed * dt;
    }

    u8 opacity = 0;
    auto state_info = get_state_info(app);

    // dissipate on hitting the boat after Spawning state
    if (state_info.state > State::Spawning) {
      if (boat.hitbox().intersects(hitbox())) {
        // Boat is in the whirlpool, apply damage if active
        damage_reduction += state_info.damage;
        should_dissipate = true;
        app.log("Boat hit whirlpool at (%.1f, %.1f), applying %u damage", x, y,
                state_info.damage);
      }
    } else if (state_info.state == State::Dead) {
      should_dissipate = true;
    }
  }

  void render(App &app, Boat &boat, Surface &region) {
    auto state_info = get_state_info(app);
    u8 opacity = state_info.opacity;
    if (opacity == 0)
      return;

    auto state = state_info.state;

    const u32 frame_idx = static_cast<u32>(
        (app.now() / whirlpool_FRAME_DURATIONS[0]) % whirlpool_FRAME_COUNT);

    // --- world -> screen ---

    i32 dst_x = static_cast<i32>(x - boat.get_x() + region.get_width() / 2);

    i32 dst_y = static_cast<i32>(boat.get_y() - y + region.get_height() / 2);

    // --- source rect ---
    i32 src_x = static_cast<i32>(frame_idx * whirlpool_FRAME_WIDTH);
    i32 src_y = 0;
    i32 w = whirlpool_FRAME_WIDTH;
    i32 h = whirlpool_FRAME_HEIGHT;

    if (!clip_blit_rect(region.get_width(), region.get_height(), dst_x, dst_y,
                        src_x, src_y, w, h))
      return;

    auto src =
        whirlpool_texture.subsurface(u32(src_x), u32(src_y), u32(w), u32(h));

    auto dst = region.subsurface(u32(dst_x), u32(dst_y), u32(w), u32(h));

    hal::gpu::blit_blend(dst, src, opacity);
  }

  enum class State {
    Spawning,
    Active,
    Dissipating,
    Dead,
  };

  struct StateInfo {
    State state;
    u8 opacity;
    u32 damage;
  };

  StateInfo get_state_info(App &app) const {
    // total alive time is 30s
    // 0->0.4: spawning, no damage, alpha ramping up from 0 to 60
    // 0.4->0.7: active, full damage, alpha from 60 to 255 (at t = 0.5)
    // 0.7->1.0: dissipating, damage decreasing, alpha ramping down
    float elapsed = ((app.now() / 1000.0f) - spawn_time) / 30;
    if (elapsed < 0.4f) {
      return {State::Spawning, static_cast<u8>((elapsed / 0.4f) * 60), 0};
    } else if (elapsed < 0.7f) {
      auto opacity =
          static_cast<u8>(60 + ((elapsed - 0.4f) / 0.3f) * (255 - 60));
      return {State::Active, opacity, 10};
    } else if (elapsed < 1.0f) {
      auto opacity = static_cast<u8>(255 * (1.0f - (elapsed - 0.7f) / 0.3f));
      return {State::Dissipating, opacity,
              static_cast<u32>(10 * ((1.0f - elapsed) / 0.3f))};
    } else {
      return {State::Dead, 0, 0};
    }
  }

private:
  float x, y;
  float spawn_time;
  TextureARGB8888 whirlpool_texture{whirlpool, whirlpool_WIDTH,
                                    whirlpool_HEIGHT, whirlpool_FORMAT_CPP};
};

class ObstacleScene : public Scene {
public:
  ObstacleScene(WorldScene &parent);

  void tick(float dt) override;
  void render(Surface &fb_region) override;

private:
  WorldScene &parent;

  ArrayVec<Whirlpool, 128> whirlpools;
};
} // namespace world
} // namespace game
} // namespace scenes
} // namespace ge
