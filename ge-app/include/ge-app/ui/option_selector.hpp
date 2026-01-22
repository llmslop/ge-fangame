#pragma once

#include "ge-app/font.hpp"
#include "ge-hal/app.hpp"
#include "ge-hal/core.hpp"
#include "ge-hal/gpu.hpp"
#include "ge-hal/surface.hpp"

namespace ge {
namespace ui {

class OptionSelector {
public:
  OptionSelector() = default;
  OptionSelector(const char *label, const char **opts, u32 num_opts,
                 u32 initial_index = 0)
      : label(label), options(opts), num_options(num_opts),
        selected_option(initial_index) {}

  void set_label(const char *lbl) { label = lbl; }

  void set_options(const char **opts, u32 num_opts, u32 initial_index = 0) {
    options = opts;
    num_options = num_opts;
    selected_option = initial_index;
  }

  void set_selected_index(u32 index) {
    if (index < num_options) {
      selected_option = index;
    }
  }

  u32 get_selected_index() const { return selected_option; }

  const char *get_selected_option() const {
    if (options && selected_option < num_options) {
      return options[selected_option];
    }
    return "";
  }

  void render(Surface &region, const Font &font, bool is_selected) {
    u16 label_color = is_selected ? 0x0000 : 0x39E7;

    // Render label
    font.render_colored(label, -1, region, 10, 2, label_color);

    // Render current option with arrows
    if (options && num_options > 0) {
      u32 option_y = font.line_height() + 6;

      // Draw left arrow if not first option
      if (selected_option > 0) {
        font.render_colored("<", -1, region, 10, option_y, label_color);
      }

      // Draw current option (centered-ish)
      const char *current_opt = options[selected_option];
      auto width = font.text_width(current_opt, -1);
      auto text_region = region.subsurface((region.get_width() - width) / 2, 0,
                                           width, region.get_height());
      font.render_colored(current_opt, -1, text_region, 0, option_y,
                          label_color);

      // Draw right arrow if not last option
      if (selected_option < num_options - 1) {
        font.render_colored(">", -1, region, region.get_width() - 20, option_y,
                            label_color);
      }
    }
  }

  // Update selection based on joystick X axis
  void adjust_selection(float joy_x) {
    // Joystick threshold for changing option
    static constexpr float JOY_THRESHOLD_MOVE = 0.5f;
    static constexpr float JOY_THRESHOLD_CENTER = 0.3f;

    if (joy_x > JOY_THRESHOLD_MOVE && !joy_moved) {
      if (selected_option < num_options - 1) {
        selected_option++;
      }
      joy_moved = true;
    } else if (joy_x < -JOY_THRESHOLD_MOVE && !joy_moved) {
      if (selected_option > 0) {
        selected_option--;
      }
      joy_moved = true;
    } else if (joy_x > -JOY_THRESHOLD_CENTER && joy_x < JOY_THRESHOLD_CENTER) {
      joy_moved = false;
    }
  }

private:
  const char *label = "";
  const char **options = nullptr;
  u32 num_options = 0;
  u32 selected_option = 0;
  bool joy_moved = false;
};

} // namespace ui
} // namespace ge
