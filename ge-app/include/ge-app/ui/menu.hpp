#pragma once

#include "ge-app/font.hpp"
#include "ge-app/gfx/shape_utils.hpp"
#include "ge-hal/app.hpp"
#include "ge-hal/core.hpp"
#include "ge-hal/gpu.hpp"
#include "ge-hal/surface.hpp"

namespace ge {
namespace ui {

struct MenuItem {
  const char *label;
  int id;
};

class Menu {
public:
  Menu() = default;

  // Initialize menu with user-provided array of items
  Menu(MenuItem *items, u32 item_count)
      : items(items), item_count(item_count) {}

  void set_items(MenuItem *items, u32 item_count) {
    this->items = items;
    this->item_count = item_count;
    selected_index = 0;
  }

  // Joystick navigation thresholds
  static constexpr float JOY_THRESHOLD_MOVE = 0.5f;
  static constexpr float JOY_THRESHOLD_CENTER = 0.3f;

  void render(Surface &region, const Font &font) {
    if (!items || item_count == 0)
      return;

    // Calculate item height and spacing
    u32 item_height = font.line_height() + 8; // 8px padding
    u32 total_height = item_count * item_height;
    u32 start_y = (region.get_height() - total_height) / 2;

    for (u32 i = 0; i < item_count; i++) {
      u32 y_pos = start_y + i * item_height;

      // Draw selection indicator
      u32 padding = 4;
      auto item_region =
          region.subsurface(padding, y_pos - padding,
                            region.get_width() - padding * 2, item_height);

      if (i == selected_index)
        draw_rect(item_region, 0x0000);

      // Just render text at left aligned position, let renderer handle it
      u16 color = (i == selected_index) ? 0x0000 : 0x39E7; // black or gray
      font.render_colored(items[i].label, -1, item_region, 10, 3, color);
    }
  }

  static void joystick_move_logic(float joy_y, bool &joy_moved,
                                  u32 &selected_index, u32 item_count) {
    if (item_count == 0)
      return;
    joy_y = -joy_y; // Invert Y axis for intuitive navigation
    if (joy_y < -JOY_THRESHOLD_MOVE && !joy_moved) {
      selected_index =
          (selected_index > 0) ? selected_index - 1 : item_count - 1;
      joy_moved = true;
    } else if (joy_y > JOY_THRESHOLD_MOVE && !joy_moved) {
      selected_index = (selected_index + 1) % item_count;
      joy_moved = true;
    } else if (joy_y > -JOY_THRESHOLD_CENTER && joy_y < JOY_THRESHOLD_CENTER) {
      // Reset joy_moved when joystick returns to center
      joy_moved = false;
    }
  }

  void move_selection(float joy_y) {
    // Use joystick Y axis to move selection
    // Only trigger on significant movement to avoid jitter
    joystick_move_logic(joy_y, joy_moved, selected_index, item_count);
  }

  int get_selected_id() const {
    if (items && selected_index < item_count) {
      return items[selected_index].id;
    }
    return -1;
  }

  u32 get_selected_index() const { return selected_index; }

private:
  MenuItem *items = nullptr;
  u32 item_count = 0;
  u32 selected_index = 0;
  bool joy_moved = false;
};

} // namespace ui
} // namespace ge
