#pragma once

#include "assets/out/textures/default-boat.h"
#include "ge-app/aabb.hpp"
#include "ge-app/game/player_stats.hpp"
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
  void render(Surface region, const PlayerStats &stats) {
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
    auto boat_region =
        region.subsurface((region.get_width() - boat.get_width()) / 2,
                          (region.get_height() - boat.get_height()) / 2,
                          boat.get_width(), boat.get_height());
    boat.blit_blend(boat_region, 0xFF);

    // render three mini 32 (region.get_width())x1 lines for HP, Food, Stamina
    auto hp_region = region.subsurface(
        (region.get_width() - boat.get_width()) / 2,
        (region.get_height() - boat.get_height()) / 2,
        boat.get_width() * stats.get_ship_hp() / stats.get_max_ship_hp(), 1);
    hal::gpu::fill(hp_region, 0xF800); // Red
    auto food_region = region.subsurface(
        (region.get_width() - boat.get_width()) / 2,
        (region.get_height() - boat.get_height()) / 2 + 1,
        boat.get_width() * stats.get_food_percent() / 100.0f, 1);
    hal::gpu::fill(food_region, 0xFBE0); // Green
    auto stamina_region = region.subsurface(
        (region.get_width() - boat.get_width()) / 2,
        (region.get_height() - boat.get_height()) / 2 + 2,
        boat.get_width() * stats.get_stamina_percent() / 100.0f, 1);
    hal::gpu::fill(stamina_region, 0x07FF); // Blue
  }

  AABB hitbox() const {
    // Simple AABB hitbox around boat
    auto texture_aabb = AABB{
        get_x(),
        get_y(),
        get_x() + static_cast<i32>(get_width()),
        get_y() + static_cast<i32>(get_height()),
    };

    // Shrink hitbox slightly, since boat sprite has some transparent padding
    return texture_aabb;
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

  void update_position(App &app, float delta_time, bool is_accelerating,
                       float cargo_weight) {
    // Calculate effective speed based on weight and acceleration
    float total_weight = ship_weight + cargo_weight;
    float weight_factor = ship_weight / total_weight; // More weight = slower

    // Base speed affected by weight
    float effective_base_speed = boat_speed * weight_factor;

    // Acceleration increases speed up to 2-3x
    if (is_accelerating) {
      // Gradually increase current speed towards max (2.5x base for this
      // weight)
      float max_accel_speed = effective_base_speed * 2.5f;
      current_speed += acceleration_rate * delta_time;
      current_speed = std::min(current_speed, max_accel_speed);
    } else {
      // Gradually decrease speed back to base
      current_speed -= deceleration_rate * delta_time;
      current_speed = std::max(current_speed, effective_base_speed);
    }

    const auto angle_val = get_angle();
    x.update(current_speed * std::cos(angle_val) * delta_time);
    y.update(current_speed * std::sin(angle_val) * delta_time);
  }

  i32 get_x() const { return x.base; }
  i32 get_y() const { return y.base; }
  float get_current_speed() const { return current_speed; }

private:
  Texture<default_boat_FORMAT_CPP> boat{
      default_boat,
      default_boat_WIDTH,
      default_boat_HEIGHT,
  };

  static constexpr float default_angle = M_PI_2;
  static constexpr float turn_rate = 1.5f;
  static constexpr float boat_speed = 30.0f;   // 30m/s base speed
  static constexpr float ship_weight = 100.0f; // Ship base weight in kg
  static constexpr float acceleration_rate =
      60.0f; // Speed increase per second when accelerating
  static constexpr float deceleration_rate =
      40.0f; // Speed decrease per second when not accelerating

  float angle = default_angle;
  float current_speed = boat_speed; // Current actual speed

  Pos<> x = 0, y = 0;
};
} // namespace ge
