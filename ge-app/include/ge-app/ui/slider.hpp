#pragma once

#include "ge-app/font.hpp"
#include "ge-app/gfx/shape_utils.hpp"
#include "ge-hal/app.hpp"
#include "ge-hal/core.hpp"
#include "ge-hal/gpu.hpp"
#include "ge-hal/surface.hpp"

namespace ge {
namespace ui {

class Slider {
public:
  Slider() = default;
  Slider(const char *label, float min_val, float max_val, float initial_val)
      : label(label), min_value(min_val), max_value(max_val),
        current_value(initial_val) {}

  void set_label(const char *lbl) { label = lbl; }

  void set_range(float min_val, float max_val) {
    min_value = min_val;
    max_value = max_val;
    // Clamp current value to new range
    if (current_value < min_value)
      current_value = min_value;
    if (current_value > max_value)
      current_value = max_value;
  }

  void set_value(float val) {
    current_value = val;
    if (current_value < min_value)
      current_value = min_value;
    if (current_value > max_value)
      current_value = max_value;
  }

  float get_value() const { return current_value; }

  // Helper to get normalized value (0.0-1.0) safely
  float get_normalized_value() const {
    float range = max_value - min_value;
    if (range == 0.0f) {
      return 1.0f; // If min equals max, consider it fully set
    }
    return (current_value - min_value) / range;
  }

  // Returns value as 0-255 range for volume control
  u8 get_value_as_volume() const {
    float normalized = get_normalized_value();
    return static_cast<u8>(normalized * 255.0f);
  }

  void render(Surface &region, const Font &font, bool is_selected) {
    u16 label_color = is_selected ? 0x0000 : 0x39E7;
    u16 bar_color = is_selected ? 0x0000 : 0x5AEB;
    u16 fill_color = is_selected ? 0x001F : 0x001F; // Blue

    // Render label
    font.render_colored(label, -1, region, 10, 2, label_color);

    // Calculate slider bar position
    u32 bar_y = font.line_height() + 6;
    u32 bar_x = 10;
    u32 bar_width = region.get_width() - 20;
    u32 bar_height = 8;

    // Draw slider background
    auto bar_region = region.subsurface(bar_x, bar_y, bar_width, bar_height);
    hal::gpu::fill(bar_region, 0xFFFF); // White background

    // Draw slider border
    draw_rect(bar_region, bar_color, 1);

    // Calculate fill width based on current value
    float normalized = get_normalized_value();
    u32 fill_width = static_cast<u32>(normalized * (bar_width - 4));

    // Draw slider fill
    if (fill_width > 0) {
      auto fill_region =
          bar_region.subsurface(2, 2, fill_width, bar_height - 4);
      hal::gpu::fill(fill_region, fill_color);
    }
  }

  // Update value based on joystick X axis
  void adjust_value(float joy_x, float step_size = 1.0f) {
    // Joystick threshold for adjustment
    static constexpr float JOY_THRESHOLD = 0.3f;

    if (joy_x > JOY_THRESHOLD) {
      current_value += step_size;
      if (current_value > max_value)
        current_value = max_value;
    } else if (joy_x < -JOY_THRESHOLD) {
      current_value -= step_size;
      if (current_value < min_value)
        current_value = min_value;
    }
  }

private:
  const char *label = "";
  float min_value = 0.0f;
  float max_value = 100.0f;
  float current_value = 50.0f;
};

} // namespace ui
} // namespace ge
