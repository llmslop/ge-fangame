#pragma once

#include "ge-app/assets/bgm.hpp"
#include "ge-app/game/boat.hpp"
#include "ge-app/game/clock.hpp"
#include "ge-app/game/compass.hpp"
#include "ge-app/game/dock.hpp"
#include "ge-app/game/fishing.hpp"
#include "ge-app/game/obstacle.hpp"
#include "ge-app/game/sky.hpp"
#include "ge-app/game/water.hpp"
#include "ge-app/gfx/color.hpp"
#include "ge-app/gfx/dialog_box.hpp"
#include "ge-app/scenes/scene.hpp"
#include <cstdlib>

namespace ge {
struct HSV {
  float h, s, v;
};

constexpr uint16_t SKY_NIGHT_RGB = 0x0821; // very dark blue
constexpr uint16_t SKY_DAY_RGB = 0x5DBF;   // clean sky blue
constexpr uint16_t SKY_WARM_RGB = 0xF2C0;  // orange / red glow

inline float smooth01(float x) {
  x = std::max(0.0f, std::min(1.0f, x));
  return x * x * (3.0f - 2.0f * x);
}

inline float lerp(float a, float b, float t) { return a + (b - a) * t; }

// Bell curve: 0 → 1 → 0
inline float bell(float x) {
  x = std::max(0.0f, std::min(1.0f, x));
  return smooth01(1.0f - std::fabs(2.0f * x - 1.0f));
}

// ------------------------------------------------------------
// Sky color function
// ------------------------------------------------------------

inline uint16_t sky_color(float t) {
  auto blend_rgb565 = [](uint16_t a, uint16_t b, float t) {
    t = std::max(0.0f, std::min(1.0f, t));
    uint8_t ti = static_cast<uint8_t>(t * 255.0f);
    return ge::blend_rgb565(a, b, ti);
  };

  t = fmodf(t, 1.0f);
  if (t < 0)
    t += 1.0f;

  constexpr float SUNRISE_START = 0.23f;
  constexpr float SUNRISE_END = 0.27f;
  constexpr float SUNSET_START = 0.73f;
  constexpr float SUNSET_END = 0.77f;

  // --- Sunrise: night → warm → day
  if (t >= SUNRISE_START && t < SUNRISE_END) {
    float k = (t - SUNRISE_START) / (SUNRISE_END - SUNRISE_START);
    k = smooth01(k);

    if (k < 0.5f)
      return blend_rgb565(SKY_NIGHT_RGB, SKY_WARM_RGB, k * 2.0f);
    else
      return blend_rgb565(SKY_WARM_RGB, SKY_DAY_RGB, (k - 0.5f) * 2.0f);
  }

  // --- Sunset: day → warm → night
  if (t >= SUNSET_START && t < SUNSET_END) {
    float k = (t - SUNSET_START) / (SUNSET_END - SUNSET_START);
    k = smooth01(k);

    if (k < 0.5f)
      return blend_rgb565(SKY_DAY_RGB, SKY_WARM_RGB, k * 2.0f);
    else
      return blend_rgb565(SKY_WARM_RGB, SKY_NIGHT_RGB, (k - 0.5f) * 2.0f);
  }

  // --- Day
  if (t >= SUNRISE_END && t < SUNSET_START)
    return SKY_DAY_RGB;

  // --- Night
  return SKY_NIGHT_RGB;
}

class GameScene : public Scene {
public:
  GameScene(App &app) : Scene{app} {
    // TODO: flash audio when it is implemented
    // Currently we skip this step to speed up flashing
#ifndef GE_HAL_STM32
    auto ambient_bgm = assets::Bgm::ambient();
    app.audio_bgm_play(ambient_bgm.data, ambient_bgm.length, true);
#endif
    sky.set_sky_color(ge::hsv_to_rgb565(150, 200, 255));
    sky.set_cloud_color(ge::hsv_to_rgb565(0, 0, 255));
    water.set_sky_color(ge::hsv_to_rgb565(150, 200, 255));
    water.set_water_color(ge::hsv_to_rgb565(142, 255, 181));

    dialog_box.show_message(app, msg[0].title, msg[0].desc);
  }

  void reset() {
    // Reset boat position and HP
    boat = Boat();
    // Clear obstacles
    obstacle_manager.clear();
    spawn_cooldown = 0.0f;
  }

  bool is_game_over() const { return !boat.is_alive(); }

  void tick(float dt) override {
    auto current_frame_world_time = clock.get_day_timer().get(app);
    float world_dt = 0.0f;
    if (last_frame_world_time >= 0) {
      world_dt = (current_frame_world_time - last_frame_world_time) * 1e-3f;
    }
    last_frame_world_time = current_frame_world_time;

    auto joystick = app.get_joystick_state();

    if (mode_indicator.get_current_mode() == GameMode::Steering) {
      if (!dialog_box.has_input_focus()) {
        boat.update_angle(joystick.x, joystick.y, world_dt);
      }
      boat.update_position(app, world_dt);
    } else if (mode_indicator.get_current_mode() == GameMode::Fishing) {
      // Update fishing system
      if (!dialog_box.has_input_focus()) {
        fishing.update(app, dialog_box, world_dt, joystick);
      }
      // Keep boat drifting slowly in fishing mode
      boat.update_position(app, world_dt);
    }

    // Check if storm is active (13:00 - 18:00, i.e., 0.542 - 0.75 of day)
    // 1PM = 13/24 = 0.542, 6PM = 18/24 = 0.75
    float time_of_day = clock.time_in_day(app);
    bool is_storm = (time_of_day >= 0.542f && time_of_day <= 0.75f);

    if (is_storm) {
      // Spawn obstacles during storm
      spawn_cooldown -= world_dt;
      if (spawn_cooldown <= 0.0f) {
        spawn_random_obstacle();
        spawn_cooldown = 2.0f; // Spawn every 2 seconds
      }
    }

    // Update obstacles
    obstacle_manager.update(world_dt, boat.get_x(), boat.get_y(),
                            ge::App::WIDTH, ge::App::HEIGHT);

    // Check collisions
    check_collisions();
  }

  void render(Surface &fb_region) override {
    auto start_time = app.now();
    auto water_region =
        fb_region.subsurface(0, 80, ge::App::WIDTH, ge::App::HEIGHT - 80);
    sky.set_x_offset(clock.get_day_timer().get(app) / 1000);
    sky.set_sky_color(sky_color(clock.time_in_day(app)));
    sky.render(fb_region.subsurface(0, 0, ge::App::WIDTH, 80));
    water.render(water_region, app.now() * 1e-3, boat.get_x(), boat.get_y());
    dock.render(app, fb_region, boat.get_x(), boat.get_y());

    // Render obstacles
    obstacle_manager.render(water_region, boat.get_x(), boat.get_y());

    font.render("Hello, World!", -1, fb_region, 10, 10,
                [](const ge::GlyphContext &g) {
                  uint8_t hue = (uint8_t)(g.x + g.gx);
                  return ge::hsv_to_rgb565(hue, 255, 255);
                });
    {
      auto max_side = std::max(boat.get_width(), boat.get_height());
      auto boat_region = water_region.subsurface(
          (water_region.get_width() - max_side) / 2,
          (water_region.get_height() - max_side) / 2, max_side, max_side);
      boat.render(boat_region);
    }
    // Render fishing line and bobber if in fishing mode
    if (mode_indicator.get_current_mode() == GameMode::Fishing &&
        fishing.is_active()) {
      // Calculate boat center position in water region
      i32 boat_center_x = 0; // Center of water region (relative coordinates)
      i32 boat_center_y = 0;
      fishing.render(water_region, boat_center_x, boat_center_y);
    }
    {
      auto compass_region =
          fb_region.subsurface(ge::App::WIDTH - compass.get_width() - 10, 10,
                               compass.get_width(), compass.get_height());
      compass.render(compass_region, boat.get_relative_angle());
    }
    {
      static constexpr u32 PADDING = 4;
      auto clock_region = fb_region.subsurface(PADDING, PADDING, 120, 15);
      clock.render(app, clock_region);
    }
    {
      static constexpr u32 PADDING = 4;
      auto mode_indicator_region =
          fb_region.subsurface(PADDING, PADDING + 16, 120, 15);
      mode_indicator.render(mode_indicator_region);
    }
    // Render HP bar
    {
      static constexpr u32 PADDING = 4;
      auto hp_region = fb_region.subsurface(PADDING, PADDING + 32, 120, 15);
      render_hp(hp_region);
    }

    {
      // bottom, padding 4px
      static constexpr auto dialog_height = 64, dialog_padding = 4;
      auto dialog_region = fb_region.subsurface(
          dialog_padding, ge::App::HEIGHT - dialog_height - dialog_padding,
          ge::App::WIDTH - dialog_padding * 2, dialog_height);
      dialog_box.render(app, dialog_region);
    }

    auto end_time = app.now();
    std::int64_t frame_time = end_time - start_time;
    // app.log("Frame time: %ld ms", frame_time);
  }
  void on_button_clicked(Button btn) override {
    if (dialog_box.has_input_focus()) {
      if (btn == Button::Button2) {
        dialog_box.dismiss();
        // tutorial messages
        if (++current_msg < sizeof(msg) / sizeof(msg[0])) {
          dialog_box.show_message(app, msg[current_msg].title,
                                  msg[current_msg].desc);
        } else {
          current_msg = sizeof(msg) / sizeof(msg[0]); // No more messages
        }
      } else if (btn == Button::Button1) {
        if (dialog_box.message_complete(app)) {
          dialog_box.dismiss();
          // tutorial messages
          if (++current_msg < sizeof(msg) / sizeof(msg[0])) {
            dialog_box.show_message(app, msg[current_msg].title,
                                    msg[current_msg].desc);
          } else {
            current_msg = sizeof(msg) / sizeof(msg[0]); // No more messages
          }
        } else {
          dialog_box.set_start_time();
        }

        return;
      }
    }

    if (btn == Button::Button2) {
      mode_indicator.switch_mode();
      clock.set_multiplier(app, mode_indicator.get_current_mode());
    } else if (btn == Button::Button1) {
      // Handle fishing dialog dismissal first (input focus)
      if (mode_indicator.get_current_mode() == GameMode::Fishing) {
        // No dialog focus, handle fishing actions
        fishing.on_button_clicked(app, dialog_box, btn);
      }
    }
  }

  void on_button_held(Button btn) override {
    if (dialog_box.has_input_focus()) {
      return; // Ignore held buttons when dialog has focus
    }

    if (btn == Button::Button2) {
      clock.begin_sped_up();
      clock.set_multiplier(app, mode_indicator.get_current_mode());
    }
  }

  void on_button_finished_hold(Button btn) override {
    if (dialog_box.has_input_focus()) {
      return; // Ignore held buttons when dialog has focus
    }

    if (btn == Button::Button2) {
      clock.end_sped_up();
      clock.set_multiplier(app, mode_indicator.get_current_mode());
    }
  }

private:
  void spawn_random_obstacle() {
    // Random obstacle type
    int type_rand = std::rand() % 10;
    ObstacleType type;
    if (type_rand < 5) {
      type = ObstacleType::Wave; // 50% waves
    } else if (type_rand < 8) {
      type = ObstacleType::Whirlpool; // 30% whirlpools
    } else {
      type = ObstacleType::Shark; // 20% sharks
    }

    if (type == ObstacleType::Wave) {
      // Waves spawn from ocean edge and move with patterns
      float angle = (std::rand() % 360) * M_PI / 180.0f;
      float spawn_distance = 200.0f + (std::rand() % 100);

      float spawn_x = boat.get_x() + spawn_distance * std::cos(angle);
      float spawn_y = boat.get_y() + spawn_distance * std::sin(angle);

      // Move towards boat
      float target_angle =
          std::atan2(boat.get_y() - spawn_y, boat.get_x() - spawn_x);
      float speed = 20.0f + (std::rand() % 20); // 20-40 m/s
      float vx = speed * std::cos(target_angle);
      float vy = speed * std::sin(target_angle);

      // Random wave pattern
      WavePattern pattern;
      int pattern_rand = std::rand() % 4;
      switch (pattern_rand) {
      case 0:
        pattern = WavePattern::Straight;
        break;
      case 1:
        pattern = WavePattern::Sine;
        break;
      case 2:
        pattern = WavePattern::Zigzag;
        break;
      case 3:
        pattern = WavePattern::Circular;
        break;
      default:
        pattern = WavePattern::Straight;
      }

      obstacle_manager.spawn_obstacle(spawn_x, spawn_y, vx, vy, type, pattern);
    } else {
      // Whirlpools and sharks appear randomly near the boat and stay in place
      float angle = (std::rand() % 360) * M_PI / 180.0f;
      float spawn_distance = 50.0f + (std::rand() % 150); // Closer than waves

      float spawn_x = boat.get_x() + spawn_distance * std::cos(angle);
      float spawn_y = boat.get_y() + spawn_distance * std::sin(angle);

      // No velocity - they don't move
      obstacle_manager.spawn_obstacle(spawn_x, spawn_y, 0.0f, 0.0f, type,
                                      WavePattern::Straight);
    }
  }

  void check_collisions() {
    auto &obstacles_vec = obstacle_manager.get_obstacles();

    // Check collision - obstacles only damage once then fade naturally
    for (auto &obstacle : obstacles_vec) {
      if (obstacle.can_damage() && 
          obstacle.collides_with(boat.get_x(), boat.get_y(), 20.0f)) {
        boat.take_damage(obstacle.get_damage());
        obstacle.mark_damaged();
      }
    }
  }

  void render_hp(Surface &region) {
    char hp_text[32];
    snprintf(hp_text, sizeof(hp_text), "HP: %.0f/%.0f", boat.get_hp(),
             boat.get_max_hp());

    hal::gpu::fill(region, 0x0000);

    // Render HP bar first
    u32 bar_width = 100;
    u32 bar_height = 8;
    u32 bar_x = 1;
    u32 bar_y = 5;

    // Draw HP bar background
    for (u32 y = bar_y; y < bar_y + bar_height && y < region.get_height();
         y++) {
      for (u32 x = bar_x; x < bar_x + bar_width && x < region.get_width();
           x++) {
        region.set_pixel(x, y, u16{0x3186}); // Dark gray
      }
    }

    // Draw HP bar foreground
    float hp_ratio = boat.get_hp() / boat.get_max_hp();
    u32 filled_width = (u32)(bar_width * hp_ratio);
    u16 hp_color =
        hp_ratio > 0.5f ? 0x07E0 : (hp_ratio > 0.25f ? 0xFFE0 : 0xF800);
    for (u32 y = bar_y; y < bar_y + bar_height && y < region.get_height();
         y++) {
      for (u32 x = bar_x; x < bar_x + filled_width && x < region.get_width();
           x++) {
        region.set_pixel(x, y, hp_color);
      }
    }
  }

  // Gameplay objects
  Compass compass;
  Boat boat;
  Clock clock;
  Timer main_timer;
  Sky sky;
  Water water;
  Fishing fishing;

  DialogBox dialog_box;
  DialogMessage msg[3] = {
      {
          "fbk",
          "Welcome to the GE-HAL and GE-App demo!\nThis "
          "is a short tutorial on how to get startedaaaaa with the game.\n",
      },
      {
          "Controls",
          "Use the joystick to steer the boat.\n"
          "Try to explore the sea and find hidden treasures!\n",
      },
      {
          "Have fun!",
          "This demo showcases basic graphics rendering, "
          "input handling, and audio playback using GE-HAL and GE-App.\n"
          "Enjoy your time on the sea!\n",
      },
  };

  u32 current_msg = 0;
  GameModeIndicator mode_indicator;
  Dock dock;
  const ge::Font &font = ge::Font::bold_font();

  i64 last_frame_world_time = -1;

  // Storm and obstacle system
  ObstacleManager obstacle_manager;
  float spawn_cooldown = 0.0f;
};
} // namespace ge
