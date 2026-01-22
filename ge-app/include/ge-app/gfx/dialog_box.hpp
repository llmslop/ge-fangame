#pragma once

#include "ge-app/font.hpp"
#include "ge-hal/app.hpp"
#include "ge-hal/gpu.hpp"
#include <cstring>
namespace ge {

struct DialogMessage {
  const char *title;
  const char *desc;
};

class DialogBox {
public:
  DialogBox() = default;

  void render(App &app, Surface region) {
    if (!has_message)
      return;
    static constexpr u32 PADDING = 4;
    hal::gpu::fill(region, 0x0000);
    region =
        region.subsurface(PADDING, PADDING, region.get_width() - PADDING * 2,
                          region.get_height() - PADDING * 2);
    const auto &bold_font = Font::bold_font();
    bold_font.render_colored(msg.title, -1, region, 0, 0, 0xFFFF);
    region = region.subsurface(0, bold_font.line_height(), region.get_width(),
                               region.get_height() - bold_font.line_height());
    u32 num_chars = (app.now() - start_time) / ms_per_char;
    Font::regular_font().render_colored(msg.desc, num_chars, region, 0, 0,
                                        0xFFFF);
  }

  // default: show everything
  void set_start_time(i64 time = -1e12) { start_time = time; }

  bool message_complete(App &app) {
    return !has_message ||
           (app.now() - start_time) / ms_per_char >= strlen(msg.desc);
  }

  // Show a message with input focus (blocks other input)
  void show_message(App &app, const char *title, const char *desc) {
    msg = {title, desc};
    has_message = true;
    start_time = app.now();
  }

  // Check if dialog has input focus
  bool has_input_focus() const { return has_message; }

  void dismiss() {
    msg = {"", ""};
    has_message = false;
  }

  // Get the current pending message
  const DialogMessage *get_pending_message() const {
    return has_message ? &msg : nullptr;
  }

private:
  i64 start_time = 0;
  i64 ms_per_char = 50;
  DialogMessage msg = {"", ""};
  bool has_message = false;
};

} // namespace ge
