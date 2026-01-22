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

// Wave movement patterns
enum class WavePattern {
  Straight,    // Move in straight line
  Sine,        // Sine wave pattern
  Zigzag,      // Zigzag pattern
  Circular     // Circular motion
};

class Obstacle {
public:
  Obstacle(float x, float y, float vx, float vy, ObstacleType type,
           float damage, WavePattern pattern = WavePattern::Straight)
      : x(x), y(y), vx(vx), vy(vy), type(type), initial_damage(damage),
        current_damage(damage), pattern(pattern), lifetime(0.0f),
        has_damaged(false) {
    // For stationary obstacles (whirlpool, shark), set max lifetime
    if (type == ObstacleType::Whirlpool) {
      max_lifetime = 8.0f; // 8 seconds to fade
    } else if (type == ObstacleType::Shark) {
      max_lifetime = 6.0f; // 6 seconds to fade
    } else {
      max_lifetime = 20.0f; // Waves last longer
    }
    
    // Store initial position for pattern calculations
    initial_x = x;
    initial_y = y;
    pattern_time = 0.0f;
  }

  void update(float dt) {
    lifetime += dt;
    
    // Update damage based on lifetime (weakening over time)
    float strength_ratio = 1.0f - (lifetime / max_lifetime);
    if (strength_ratio < 0.0f) strength_ratio = 0.0f;
    current_damage = initial_damage * strength_ratio;
    
    if (type == ObstacleType::Wave) {
      // Waves move with patterns
      pattern_time += dt;
      
      switch (pattern) {
      case WavePattern::Straight:
        x += vx * dt;
        y += vy * dt;
        break;
        
      case WavePattern::Sine:
        // Move forward and oscillate perpendicular
        x += vx * dt;
        y += vy * dt;
        // Add sine wave perpendicular to direction
        {
          float perp_x = -vy / std::sqrt(vx * vx + vy * vy);
          float perp_y = vx / std::sqrt(vx * vx + vy * vy);
          float oscillation = std::sin(pattern_time * 3.0f) * 15.0f;
          x += perp_x * oscillation * dt;
          y += perp_y * oscillation * dt;
        }
        break;
        
      case WavePattern::Zigzag:
        // Zigzag pattern
        x += vx * dt;
        y += vy * dt;
        {
          float perp_x = -vy / std::sqrt(vx * vx + vy * vy);
          float perp_y = vx / std::sqrt(vx * vx + vy * vy);
          float zigzag = (int(pattern_time * 2.0f) % 2 == 0) ? 10.0f : -10.0f;
          x += perp_x * zigzag * dt;
          y += perp_y * zigzag * dt;
        }
        break;
        
      case WavePattern::Circular:
        // Circular/spiral motion
        {
          float base_speed = std::sqrt(vx * vx + vy * vy);
          float radius = 30.0f;
          float angular_speed = base_speed / radius;
          pattern_time += dt;
          
          float center_vx = vx;
          float center_vy = vy;
          x += center_vx * dt;
          y += center_vy * dt;
          
          // Add circular component
          float circle_x = std::cos(pattern_time * angular_speed) * radius;
          float circle_y = std::sin(pattern_time * angular_speed) * radius;
          x += (circle_x - std::cos((pattern_time - dt) * angular_speed) * radius);
          y += (circle_y - std::sin((pattern_time - dt) * angular_speed) * radius);
        }
        break;
      }
    }
    // Whirlpools and sharks don't move - they stay in place and fade
  }

  float get_x() const { return x; }
  float get_y() const { return y; }
  ObstacleType get_type() const { return type; }
  float get_damage() const { return current_damage; }
  
  bool can_damage() const { return !has_damaged && current_damage > 0.0f; }
  
  void mark_damaged() { has_damaged = true; }
  
  bool should_remove() const {
    return lifetime >= max_lifetime || current_damage <= 0.0f;
  }
  
  float get_alpha() const {
    // Fade out as lifetime approaches max
    float alpha = 1.0f - (lifetime / max_lifetime);
    return alpha > 0.0f ? alpha : 0.0f;
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
    
    float alpha = get_alpha();
    if (alpha <= 0.0f) return;

    // Render based on type
    u16 color;
    i32 size = (i32)get_radius();

    switch (type) {
    case ObstacleType::Wave:
      color = 0xFFFF; // White
      // Apply alpha blending for fading
      if (alpha < 1.0f) {
        // Approximate alpha by making color lighter
        u8 blend = (u8)(255 * alpha);
        color = ge::blend_rgb565(0x18E3, 0xFFFF, blend); // Blend with water color
      }
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
      // Apply alpha - scale size based on alpha
      size = (i32)(size * alpha);
      if (size < 5) size = 5;
      
      // Draw whirlpool as a circle with spiral pattern
      {
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
                  pixel_color = u16{0x0010}; // Darker blue
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
      color = 0x7800; // Gray
      // Apply alpha - make smaller and fade
      size = (i32)(size * alpha);
      if (size < 5) size = 5;
      
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
      // Draw fin (small triangle on top) - only if alpha is high enough
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
      break;
    }
  }

private:
  float x, y;   // World position
  float vx, vy; // Velocity (only used for waves)
  ObstacleType type;
  float initial_damage;
  float current_damage;  // Weakens over time
  WavePattern pattern;
  float lifetime;
  float max_lifetime;
  float initial_x, initial_y;  // For pattern calculations
  float pattern_time;
  bool has_damaged;  // Track if this obstacle has already damaged the boat
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

  void spawn_obstacle(float x, float y, float vx, float vy, ObstacleType type,
                      WavePattern pattern = WavePattern::Straight) {
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
    obstacles.emplace_back(x, y, vx, vy, type, damage, pattern);
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
