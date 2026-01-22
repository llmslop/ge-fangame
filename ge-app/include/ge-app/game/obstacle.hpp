#pragma once

#include "ge-app/gfx/color.hpp"
#include "ge-hal/app.hpp"
#include "ge-hal/surface.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace ge {

enum class ObstacleType { Wave, Whirlpool, Shark };

class Obstacle {
public:
  Obstacle(float x, float y, float vx, float vy, ObstacleType type,
           float damage)
      : x(x), y(y), vx(vx), vy(vy), type(type), damage(damage) {}

  void update(float dt) {
    x += vx * dt;
    y += vy * dt;
  }

  float get_x() const { return x; }
  float get_y() const { return y; }
  ObstacleType get_type() const { return type; }
  float get_damage() const { return damage; }

  // Simple collision detection - check if point (px, py) is within radius
  bool collides_with(float px, float py, float collision_radius) const {
    float dx = x - px;
    float dy = y - py;
    float dist_sq = dx * dx + dy * dy;
    float total_radius = get_radius() + collision_radius;
    return dist_sq < (total_radius * total_radius);
  }

  float get_radius() const {
    switch (type) {
    case ObstacleType::Wave:
      return 20.0f;
    case ObstacleType::Whirlpool:
      return 25.0f;
    case ObstacleType::Shark:
      return 15.0f;
    }
    return 20.0f;
  }

  // Check if obstacle is far off-screen and should be removed
  bool is_off_screen(i32 boat_x, i32 boat_y, u32 screen_width,
                     u32 screen_height) const {
    float dx = x - boat_x;
    float dy = y - boat_y;
    float max_dist = std::max(screen_width, screen_height) + 100.0f;
    return std::abs(dx) > max_dist || std::abs(dy) > max_dist;
  }

  void render(Surface &region, i32 boat_x, i32 boat_y) const {
    // Convert world position to screen position
    i32 screen_x = (i32)(x - boat_x) + region.get_width() / 2;
    i32 screen_y = (i32)(y - boat_y) + region.get_height() / 2;

    // Skip if off-screen
    if (screen_x < -50 || screen_x > (i32)region.get_width() + 50 ||
        screen_y < -50 || screen_y > (i32)region.get_height() + 50) {
      return;
    }

    // Render based on type
    u16 color;
    i32 size = (i32)get_radius();

    switch (type) {
    case ObstacleType::Wave:
      color = 0xFFFF; // White
      // Draw wave as a horizontal ellipse
      for (i32 dy = -size / 2; dy <= size / 2; dy++) {
        i32 width =
            (i32)(size * 1.5f *
                  std::sqrt(1.0f - (dy * dy) / (float)(size * size / 4)));
        for (i32 dx = -width; dx <= width; dx++) {
          i32 px = screen_x + dx;
          i32 py = screen_y + dy;
          if (px >= 0 && px < (i32)region.get_width() && py >= 0 &&
              py < (i32)region.get_height()) {
            region.set_pixel(px, py, color);
          }
        }
      }
      break;

    case ObstacleType::Whirlpool:
      color = 0x001F; // Deep blue
      // Draw whirlpool as a circle with spiral pattern
      {
        float size_sq_over_4 = (float)(size * size) / 4.0f;
        for (i32 dy = -size; dy <= size; dy++) {
          for (i32 dx = -size; dx <= size; dx++) {
            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist <= size) {
              i32 px = screen_x + dx;
              i32 py = screen_y + dy;
              if (px >= 0 && px < (i32)region.get_width() && py >= 0 &&
                  py < (i32)region.get_height()) {
                // Create spiral effect
                float angle = std::atan2(dy, dx);
                float spiral = std::fmod(angle + dist * 0.2f, M_PI / 4);
                if (spiral < M_PI / 8) {
                  region.set_pixel(px, py, u16{0x0010}); // Darker blue
                } else {
                  region.set_pixel(px, py, color);
                }
              }
            }
          }
        }
      }
      break;

    case ObstacleType::Shark:
      color = 0x7800; // Gray
      // Draw shark as a triangle
      for (i32 dy = -size; dy <= size; dy++) {
        i32 width = size - std::abs(dy);
        for (i32 dx = -width; dx <= width; dx++) {
          i32 px = screen_x + dx;
          i32 py = screen_y + dy;
          if (px >= 0 && px < (i32)region.get_width() && py >= 0 &&
              py < (i32)region.get_height()) {
            region.set_pixel(px, py, color);
          }
        }
      }
      // Draw fin (small triangle on top)
      for (i32 dy = -size - 5; dy <= -size; dy++) {
        i32 width = (-size - dy) / 2;
        for (i32 dx = -width; dx <= width; dx++) {
          i32 px = screen_x + dx;
          i32 py = screen_y + dy;
          if (px >= 0 && px < (i32)region.get_width() && py >= 0 &&
              py < (i32)region.get_height()) {
            region.set_pixel(px, py, color);
          }
        }
      }
      break;
    }
  }

private:
  float x, y;   // World position
  float vx, vy; // Velocity
  ObstacleType type;
  float damage;
};

class ObstacleManager {
public:
  void update(float dt, i32 boat_x, i32 boat_y, u32 screen_width,
              u32 screen_height) {
    // Update all obstacles
    for (auto &obstacle : obstacles) {
      obstacle.update(dt);
    }

    // Remove off-screen obstacles
    obstacles.erase(std::remove_if(obstacles.begin(), obstacles.end(),
                                   [boat_x, boat_y, screen_width,
                                    screen_height](const Obstacle &obs) {
                                     return obs.is_off_screen(boat_x, boat_y,
                                                              screen_width,
                                                              screen_height);
                                   }),
                    obstacles.end());
  }

  void spawn_obstacle(float x, float y, float vx, float vy, ObstacleType type) {
    float damage;
    switch (type) {
    case ObstacleType::Wave:
      damage = 5.0f;
      break;
    case ObstacleType::Whirlpool:
      damage = 8.0f;
      break;
    case ObstacleType::Shark:
      damage = 90.0f; // Almost instant kill
      break;
    }
    obstacles.emplace_back(x, y, vx, vy, type, damage);
  }

  void render(Surface &region, i32 boat_x, i32 boat_y) const {
    for (const auto &obstacle : obstacles) {
      obstacle.render(region, boat_x, boat_y);
    }
  }

  const std::vector<Obstacle> &get_obstacles() const { return obstacles; }
  std::vector<Obstacle> &get_obstacles() { return obstacles; }

  void clear() { obstacles.clear(); }

private:
  std::vector<Obstacle> obstacles;
};

} // namespace ge
