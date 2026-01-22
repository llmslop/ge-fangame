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

// Wave placement patterns (not movement patterns)
enum class WavePlacementPattern {
  Single,      // Single wave
  Barrier,     // Line of waves with gaps
  Scattered,   // Random scattered waves
  Convergent   // Waves converging from multiple directions
};

class Obstacle {
public:
  Obstacle(float x, float y, float vx, float vy, ObstacleType type,
           float damage)
      : x(x), y(y), vx(vx), vy(vy), type(type), initial_damage(damage),
        current_damage(0.0f), lifetime(0.0f), has_damaged(false) {
    // Set growth time and max lifetime based on type
    if (type == ObstacleType::Whirlpool) {
      growth_time = 2.0f;    // 2 seconds to grow
      max_lifetime = 10.0f;  // 10 seconds total (2s grow + 8s active)
    } else if (type == ObstacleType::Shark) {
      growth_time = 1.5f;    // 1.5 seconds warning
      max_lifetime = 7.5f;   // 7.5 seconds total (1.5s warn + 6s active)
    } else {
      growth_time = 0.5f;    // 0.5 seconds to grow
      max_lifetime = 20.5f;  // 20.5 seconds total
    }
    
    initial_x = x;
    initial_y = y;
  }

  void update(float dt) {
    lifetime += dt;
    
    // Growth phase - obstacle grows to full strength
    if (lifetime < growth_time) {
      float growth_ratio = lifetime / growth_time;
      current_damage = initial_damage * growth_ratio;
    } else {
      // Active phase - full strength, then weakening
      float active_time = lifetime - growth_time;
      float max_active_time = max_lifetime - growth_time;
      float strength_ratio = 1.0f - (active_time / max_active_time);
      if (strength_ratio < 0.0f) strength_ratio = 0.0f;
      current_damage = initial_damage * strength_ratio;
    }
    
    // Only waves move - sharks and whirlpools are stationary
    if (type == ObstacleType::Wave) {
      x += vx * dt;
      y += vy * dt;
    }
  }

  float get_x() const { return x; }
  float get_y() const { return y; }
  ObstacleType get_type() const { return type; }
  float get_damage() const { return current_damage; }
  
  bool can_damage() const { 
    // Can only damage after growth phase
    return !has_damaged && lifetime >= growth_time && current_damage > 0.0f; 
  }
  
  void mark_damaged() { has_damaged = true; }
  
  bool should_remove() const {
    return lifetime >= max_lifetime || current_damage <= 0.0f;
  }
  
  float get_growth_ratio() const {
    if (lifetime < growth_time) {
      return lifetime / growth_time;
    }
    return 1.0f;
  }
  
  float get_alpha() const {
    // During growth phase, fade in
    if (lifetime < growth_time) {
      return lifetime / growth_time;
    }
    // After growth, fade out based on remaining lifetime
    float active_time = lifetime - growth_time;
    float max_active_time = max_lifetime - growth_time;
    float alpha = 1.0f - (active_time / max_active_time);
    return alpha > 0.0f ? alpha : 0.0f;
  }
  
  bool is_warning_phase() const {
    // Warning phase is during growth
    return lifetime < growth_time;
  }

  // Simple collision detection - check if point (px, py) is within radius
  bool collides_with(float px, float py, float collision_radius) const {
    float dx = x - px;
    float dy = y - py;
    float dist_sq = dx * dx + dy * dy;
    float total_radius = get_radius() + collision_radius;
    return dist_sq < (total_radius * total_radius);
  }

  float get_radius() const {
    float base_radius;
    switch (type) {
    case ObstacleType::Wave:
      base_radius = 20.0f;
      break;
    case ObstacleType::Whirlpool:
      base_radius = 25.0f;
      break;
    case ObstacleType::Shark:
      base_radius = 15.0f;
      break;
    default:
      base_radius = 20.0f;
    }
    
    // Scale by growth ratio
    return base_radius * get_growth_ratio();
  }

  // Check if obstacle is far off-screen and should be removed
  bool is_off_screen(i32 boat_x, i32 boat_y, u32 screen_width,
                     u32 screen_height) const {
    // Only check for waves (whirlpools and sharks are stationary)
    if (type != ObstacleType::Wave) {
      return false;
    }
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
    
    float growth = get_growth_ratio();
    float alpha = get_alpha();
    if (alpha <= 0.0f) return;
    
    bool warning = is_warning_phase();

    // Render based on type
    u16 color;
    i32 size = (i32)(get_radius());
    if (size < 3) size = 3;

    switch (type) {
    case ObstacleType::Wave:
      color = 0xFFFF; // White
      // Apply alpha blending for fading
      if (alpha < 1.0f) {
        u8 blend = (u8)(255 * alpha);
        color = ge::blend_rgb565(0x18E3, 0xFFFF, blend);
      }
      // Draw wave as a horizontal ellipse (scaled by growth)
      for (i32 dy = -size / 2; dy <= size / 2; dy++) {
        i32 width = (i32)(size * 1.5f * growth *
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
      // During warning phase, show pulsing circle indicator
      if (warning) {
        // Pulsing warning indicator
        float pulse = 0.5f + 0.5f * std::sin(lifetime * 8.0f);
        u16 warn_color = ge::blend_rgb565(0x001F, 0x07FF, (u8)(pulse * 255));
        i32 warn_size = (i32)(size * (0.5f + 0.5f * growth));
        
        // Draw pulsing circle
        for (i32 dy = -warn_size; dy <= warn_size; dy++) {
          for (i32 dx = -warn_size; dx <= warn_size; dx++) {
            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist <= warn_size && dist >= warn_size - 3) {
              i32 px = screen_x + dx;
              i32 py = screen_y + dy;
              if (px >= 0 && px < (i32)region.get_width() && py >= 0 &&
                  py < (i32)region.get_height()) {
                region.set_pixel(px, py, warn_color);
              }
            }
          }
        }
      } else {
        // Full whirlpool with spiral
        color = 0x001F; // Deep blue
        
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
                float spiral = std::fmod(angle + dist * 0.2f + lifetime * 2.0f, M_PI / 4);
                u16 pixel_color;
                if (spiral < M_PI / 8) {
                  pixel_color = u16{0x0010};
                } else {
                  pixel_color = color;
                }
                // Apply alpha blending
                if (alpha < 1.0f) {
                  u8 blend = (u8)(255 * alpha);
                  pixel_color = ge::blend_rgb565(0x18E3, pixel_color, blend);
                }
                region.set_pixel(px, py, pixel_color);
              }
            }
          }
        }
      }
      break;

    case ObstacleType::Shark:
      // During warning phase, show fin circling indicator
      if (warning) {
        // Circling fin warning
        float circle_angle = lifetime * 4.0f; // Circle 4 times during warning
        i32 circle_radius = size + 10;
        i32 fin_x = screen_x + (i32)(std::cos(circle_angle) * circle_radius);
        i32 fin_y = screen_y + (i32)(std::sin(circle_angle) * circle_radius);
        
        // Draw small warning fin
        u16 warn_color = 0xF800; // Red warning
        i32 fin_size = (i32)(5 * growth);
        for (i32 dy = -fin_size; dy <= 0; dy++) {
          i32 width = fin_size + dy;
          for (i32 dx = -width; dx <= width; dx++) {
            i32 px = fin_x + dx;
            i32 py = fin_y + dy;
            if (px >= 0 && px < (i32)region.get_width() && py >= 0 &&
                py < (i32)region.get_height()) {
              region.set_pixel(px, py, warn_color);
            }
          }
        }
      } else {
        // Full shark bite
        color = 0x7800; // Gray
        
        // Blend color for fading
        if (alpha < 1.0f) {
          u8 blend = (u8)(255 * alpha);
          color = ge::blend_rgb565(0x18E3, color, blend);
        }
        
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
        // Draw fin
        if (alpha > 0.3f) {
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
        }
      }
      break;
    }
  }

private:
  float x, y;   // World position
  float vx, vy; // Velocity (only used for waves)
  ObstacleType type;
  float initial_damage;
  float current_damage;  // Grows then weakens over time
  float lifetime;
  float growth_time;     // Time to reach full size
  float max_lifetime;
  float initial_x, initial_y;
  bool has_damaged;
};

class ObstacleManager {
public:
  void update(float dt, i32 boat_x, i32 boat_y, u32 screen_width,
              u32 screen_height) {
    // Update all obstacles
    for (auto &obstacle : obstacles) {
      obstacle.update(dt);
    }

    // Remove obstacles that should be removed (expired or off-screen)
    obstacles.erase(
        std::remove_if(obstacles.begin(), obstacles.end(),
                       [boat_x, boat_y, screen_width, screen_height](
                           const Obstacle &obs) {
                         return obs.should_remove() ||
                                obs.is_off_screen(boat_x, boat_y, screen_width,
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
