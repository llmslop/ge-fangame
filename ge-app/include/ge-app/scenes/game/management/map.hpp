#pragma once

#include "ge-app/font.hpp"
#include "ge-app/game/boat.hpp"
#include "ge-app/game/clock.hpp"
#include "ge-app/game/dock.hpp"
#include "ge-app/game/water.hpp"
#include "ge-app/scenes/base.hpp"
#include "ge-hal/app.hpp"
#include "ge-hal/gpu.hpp"
#include "ge-hal/surface.hpp"
#include <cstdio>

namespace ge {
namespace scenes {
namespace game {
class ManagementUIScene;
namespace mgmt {

struct Bookmark {
  char name[32];
  i32 x;
  i32 y;
  char icon;
  bool active;

  Bookmark() : x(0), y(0), icon('*'), active(false) { name[0] = '\0'; }
};

class MapScene : public Scene {
public:
  MapScene(ManagementUIScene &parent);

  void on_enter() {
    // set map_offset to boat position
    map_offset_x = static_cast<float>(get_boat_x());
    map_offset_y = static_cast<float>(get_boat_y());
  }

  bool on_joystick_moved(float dt, float x, float y) override {
    // Handle navigation with joystick
    static constexpr float MOVE_THRESHOLD = 0.5f;
    static constexpr float CENTER_THRESHOLD = 0.3f;
    static constexpr float MAP_MOVE_SPEED = 1000.0f; // pixels per second

    float dx = std::abs(x) > MOVE_THRESHOLD ? x : 0.0f;
    float dy = std::abs(y) > MOVE_THRESHOLD ? y : 0.0f;

    map_offset_x += dx * MAP_MOVE_SPEED * dt;
    map_offset_y += dy * MAP_MOVE_SPEED * dt;

    apply_y_clamp();
    return true;
  }

  void render(Surface &fb_region) override {
    // Render ocean texture
    water.render(nullptr, fb_region, NAN,
                 static_cast<u32>(static_cast<i32>(map_offset_x) / SCALE_DOWN),
                 static_cast<u32>(static_cast<i32>(map_offset_y)) / SCALE_DOWN);

    // Render dock (dock color fill) for from y=0 downward
    u16 dock_color = Dock::map_color(); // light blue

    // Calculate crosshair position (center of screen for map view)
    u32 screen_center_x = fb_region.get_width() / 2;
    u32 screen_center_y = fb_region.get_height() / 2;

    // Project world Y = -40 into screen space
    i32 dock_top =
        screen_center_y - (-40 - static_cast<i32>(map_offset_y)) / SCALE_DOWN;

    // Clamp and fill everything below
    if (dock_top < fb_region.get_height()) {
      i32 y = std::max<i32>(dock_top, 0);
      i32 h = fb_region.get_height() - y;

      if (h > 0) {
        auto dock_region = fb_region.subsurface(0, y, fb_region.get_width(), h);
        hal::gpu::fill(dock_region, dock_color);
      }
    }

    const auto &font = Font::regular_font();
    const u32 line_height = font.line_height() + 2;

    // Draw title
    font.render_colored("=== World Map ===", -1, fb_region, 70, 5, 0xFFFF);

    // Get current boat position
    i32 boat_x = get_boat_x();
    i32 boat_y = get_boat_y();

    // Calculate crosshair world coordinates
    i32 crosshair_x = static_cast<i32>(map_offset_x);
    i32 crosshair_y = static_cast<i32>(map_offset_y);

    // Draw crosshair at center of screen
    font.render_colored("+", -1, fb_region, screen_center_x - 3,
                        screen_center_y - 4, 0xFFFF);

    // Draw boat position on map
    i32 boat_rel_x = boat_x - crosshair_x / SCALE_DOWN;
    i32 boat_rel_y = -(boat_y - crosshair_y) / SCALE_DOWN;
    i32 boat_screen_x = screen_center_x + boat_rel_x;
    i32 boat_screen_y = screen_center_y + boat_rel_y;

    // Only draw boat marker if on screen
    if (boat_screen_x >= 0 && boat_screen_x < (i32)fb_region.get_width() &&
        boat_screen_y >= 0 && boat_screen_y < (i32)fb_region.get_height()) {
      // Temp marker (red square)
      hal::gpu::fill(fb_region.subsurface(boat_screen_x, boat_screen_y, 4, 4),
                     0xF800);
    }

    // Draw bookmarks
    for (u32 i = 0; i < MAX_BOOKMARKS; i++) {
      if (bookmarks[i].active) {
        // Calculate screen position for bookmark
        i32 rel_x = (bookmarks[i].x - crosshair_x) / SCALE_DOWN;
        i32 rel_y = -(bookmarks[i].y - crosshair_y) / SCALE_DOWN;

        i32 screen_x = screen_center_x + rel_x;
        i32 screen_y = screen_center_y + rel_y;

        auto dist = dist_to_bookmark(bookmarks[i].x, bookmarks[i].y,
                                     crosshair_x, crosshair_y);

        // Only draw if on screen
        if (screen_x >= 0 && screen_x < (i32)fb_region.get_width() &&
            screen_y >= 0 && screen_y < (i32)fb_region.get_height()) {
          char icon_str[2] = {bookmarks[i].icon, '\0'};
          // TODO: replace this with texture
          font.render_colored(icon_str, -1, fb_region, screen_x, screen_y,
                              0xFFFF); // white
        }

        // render label on top
        u16 color = (dist < DIST_THRESHOLD)
                        ? 0xF800
                        : 0xFFE0; // Red if close, else yellow
        auto width = font.text_width(bookmarks[i].name, -1);
        auto x = screen_x - (i32)(width / 2);
        font.render_colored(bookmarks[i].name, -1, fb_region, x,
                            screen_y - line_height, color);
      }
    }

    // Display coordinates at top
    char coord_buf[64];
    snprintf(coord_buf, sizeof(coord_buf), "Boat: (%d, %d)", boat_x, boat_y);
    font.render_colored(coord_buf, -1, fb_region, 10, 25, 0x7BEF);

    snprintf(coord_buf, sizeof(coord_buf), "Cross: (%d, %d)", crosshair_x,
             crosshair_y);
    font.render_colored(coord_buf, -1, fb_region, 10, 25 + line_height, 0x7BEF);

    u32 active_count = 0;
    for (u32 i = 0; i < MAX_BOOKMARKS; i++) {
      if (bookmarks[i].active) {
        active_count++;
      }
    }

    // Instructions
    char instr_buf[64];
    snprintf(instr_buf, sizeof(instr_buf), "A: Add Bookmark (%d/%d)  B: Return",
             active_count, MAX_BOOKMARKS);
    font.render_colored(instr_buf, -1, fb_region, 10,
                        fb_region.get_height() - line_height - 5, 0x7BEF);
  }

  bool on_button_clicked(Button btn) override {
    if (btn == Button::Button1) {
      // Add bookmark at crosshair position
      if (remove_bookmark_at(map_offset_x, map_offset_y)) {
        return true;
      }
      bool added = add_bookmark();
      if (!added) {
        // All slots full - remove oldest and add new one
        remove_first_bookmark();
        add_bookmark();
      }
      return true;
    } else if (btn == Button::Button2) {
      // Return to management menu
      on_back_action();
      return true;
    }
    return Scene::on_button_clicked(btn);
  }

  void on_back_action();

  bool is_active() const override;

private:
  ManagementUIScene &parent;
  Water water;

  static constexpr u32 MAX_BOOKMARKS = 32;
  static constexpr i32 SCALE_DOWN = 10; // 10px in world = 1px on map
  static constexpr i32 MIN_Y = -100;    // Minimum Y coordinate

  Bookmark bookmarks[MAX_BOOKMARKS];
  float map_offset_x = 0.0f;
  float map_offset_y = 0.0f;
  bool joy_moved_x = false;
  bool joy_moved_y = false;

  i32 get_boat_x();
  i32 get_boat_y();
  Clock &get_clock();

  bool add_bookmark() {
    // Find first empty slot
    for (u32 i = 0; i < MAX_BOOKMARKS; i++) {
      if (!bookmarks[i].active) {
        bookmarks[i].active = true;
        bookmarks[i].x = static_cast<i32>(map_offset_x);
        bookmarks[i].y = static_cast<i32>(map_offset_y);
        bookmarks[i].icon = '0' + i; // Use 0-4 as icons

        // Generate bookmark name based on current time
        auto &clock = get_clock();
        u32 day = clock.get_num_days(app) + 1;
        u32 hr = clock.get_hr(app);
        const char *am_pm = (hr >= 12) ? "PM" : "AM";
        u32 display_hr = (hr % 12 == 0) ? 12 : (hr % 12);

        snprintf(bookmarks[i].name, sizeof(bookmarks[i].name), "Day %u %u %s",
                 day, display_hr, am_pm);
        return true;
      }
    }
    return false; // All slots full
  }

  static constexpr u32 DIST_THRESHOLD = 16 * SCALE_DOWN;

  u32 dist_to_bookmark(i32 x1, i32 y1, i32 x2, i32 y2) {
    return std::abs(x1 - x2) + std::abs(y1 - y2);
  }

  bool remove_bookmark_at(i32 x, i32 y) {
    for (u32 i = 0; i < MAX_BOOKMARKS; i++) {
      auto dist = dist_to_bookmark(bookmarks[i].x, bookmarks[i].y, x, y);
      if (dist < DIST_THRESHOLD && bookmarks[i].active) {
        bookmarks[i].active = false;
        return true;
      }
    }

    return false;
  }

  void remove_first_bookmark() {
    // Remove the first active bookmark (oldest)
    for (u32 i = 0; i < MAX_BOOKMARKS; i++) {
      if (bookmarks[i].active) {
        bookmarks[i].active = false;
        return;
      }
    }
  }

  void apply_y_clamp() {
    // Clamp Y to minimum value
    if (map_offset_y < MIN_Y) {
      map_offset_y = MIN_Y;
    }
    // X is unlimited (no clamping)
  }
};

} // namespace mgmt
} // namespace game
} // namespace scenes
} // namespace ge
