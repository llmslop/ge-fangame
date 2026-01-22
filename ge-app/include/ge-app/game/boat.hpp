#pragma once

#include "assets/out/textures/default-boat.h"
#include "ge-app/texture.hpp"
#include "ge-hal/app.hpp"
#include <cmath>

namespace ge {
template <class T = i32, class Rem = u32, Rem RemMax = 4096> struct Pos {
  T base;
  Rem rem;
  // real value = base + rem / RemMax

  constexpr Pos(i32 base) : base(base), rem(0) {}

  void update(float delta) {
    i64 to_add = delta * RemMax;
    i64 new_rem = static_cast<i64>(rem) + to_add;
    if (new_rem > 0) {
      base += static_cast<T>(new_rem / RemMax);
      rem = static_cast<Rem>(new_rem % RemMax);
    } else {
      i64 borrow = (-new_rem + RemMax - 1) / RemMax;
      base -= static_cast<T>(borrow);
      rem = static_cast<Rem>(new_rem + borrow * RemMax);
    }
  }
};

class Boat {
public:
  u32 get_width() const { return boat.get_width(); }
  u32 get_height() const { return boat.get_height(); }
  void render(Surface &region) {
    auto angle_to_8dir = [](float angle) {
      // normalize to [0, 2π)
      angle = std::fmod(angle, 2.0f * M_PI);
      if (angle < 0)
        angle += 2.0f * M_PI;

      // add half-sector to round correctly
      angle += M_PI / 8.0f;

      // map to 0..7
      int dir = int(angle / (M_PI / 4.0f)) & 7;
      return dir;
    };

    // TODO: implement rotation (probably via sprite sheet for performance)
    // float angle = -angle_to_8dir(get_relative_angle()) * M_PI_4;
    // boat.blit_rotated(region, region.get_width() / 2, region.get_height() /
    // 2,
    //                   angle);
    //
    // region is a 64x64 region, but the boat texture is 32x64 only
    region = region.subsurface((region.get_width() - boat.get_width()) / 2,
                               (region.get_height() - boat.get_height()) / 2,
                               boat.get_width(), boat.get_height());
    boat.blit(region);
  }

  float get_angle() const { return angle; }
  float get_relative_angle() const { return angle - default_angle; }

  void update_angle(float joystick_x, float joystick_y, float delta_time) {
    auto angle_diff = [](float from, float to) {
      float d = std::fmod(to - from + M_PI, 2.0f * M_PI);
      if (d < 0)
        d += 2.0f * M_PI;
      return d - M_PI;
    };

    float x = joystick_x, y = joystick_y;
    float mag = std::sqrt(x * x + y * y);

    if (mag > 0.0f) {
      float target = std::atan2(y, x);
      float diff = angle_diff(angle, target);
      float max_step = turn_rate * delta_time;
      diff = std::max(std::min(diff, max_step), -max_step);
      angle += diff;
    }

    angle = std::fmod(angle, 2.0f * M_PI);
  }

  void update_position(App &app, float delta_time) {
    const auto angle = get_angle();
    x.update(boat_speed * std::cos(angle) * delta_time);
    y.update(boat_speed * std::sin(angle) * delta_time);
  }

  i32 get_x() const { return x.base; }
  i32 get_y() const { return y.base; }

  // HP system
  float get_hp() const { return hp; }
  float get_max_hp() const { return max_hp; }
  void take_damage(float damage) {
    hp -= damage;
    if (hp < 0.0f)
      hp = 0.0f;
  }
  bool is_alive() const { return hp > 0.0f; }

private:
  Texture boat{
      default_boat,
      default_boat_WIDTH,
      default_boat_HEIGHT,
  };

  static constexpr float default_angle = M_PI_2, turn_rate = 1.5f,
                         boat_speed = 30; // 30m/s
  float angle = default_angle;

  Pos<> x = 0, y = 0;

  // HP system
  static constexpr float max_hp = 100.0f;
  float hp = max_hp;
};
} // namespace ge
