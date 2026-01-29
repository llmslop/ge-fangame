#pragma once

#include "ge-app/font.hpp"
#include "ge-app/game/mode_indicator.hpp"
#include "ge-app/timer.hpp"
#include "ge-hal/app.hpp"
#include "ge-hal/core.hpp"
#include "ge-hal/gpu.hpp"
#include <cstdio>

namespace ge {
class Clock {
public:
  u32 get_num_days(App &app) const { return day_timer.get(app) / DAY_LENGTH; }

  u32 get_hr(App &app) const {
    auto elapsed = day_timer.get(app) % DAY_LENGTH;
    static constexpr u32 HR_PER_DAY = 24;
    u32 hr = (elapsed * HR_PER_DAY) / DAY_LENGTH;
    return hr;
  }

  char *get_display_string(App &app) const {
    static char buf[64] = {0};
    u32 hr = get_hr(app);
    const char *am_pm = (hr >= 12) ? "PM" : "AM";
    snprintf(buf, sizeof(buf), "Day %u, %u %s (%.1fx)", get_num_days(app) + 1,
             (hr % 12 == 0) ? 12 : (hr % 12), am_pm,
             (float)day_timer.get_multiplier_num() /
                 day_timer.get_multiplier_den());
    return buf;
  }

  void render(App &app, const Surface &region) {
    auto str = get_display_string(app);
    hal::gpu::fill(region, 0x0000);
    Font::regular_font().render_colored(str, -1, region, 1, 1, 0xFFFF);
  }

  float time_in_day(App &app) {
    auto elapsed = day_timer.get(app) % DAY_LENGTH;
    return float(elapsed) / float(DAY_LENGTH);
  }

  void update_multiplier(App &app, GameMode mode) {
    auto multiplier = game_mode_speed_multiplier(mode);
    day_timer.set_multiplier(app, multiplier.first * (sped_up ? 3 : 1),
                             multiplier.second);
  }

  void begin_sped_up() { sped_up = true; }
  void end_sped_up() { sped_up = false; }

  const Timer &get_game_timer() const { return day_timer; }

  void reset() {
    // start at 6AM
    day_timer.reset(DAY_LENGTH / 4);
    sped_up = false;
  }

  static constexpr i64 DAY_LENGTH =
      180 * 1000; // each day is 3 minute in real time
private:
  Timer day_timer;
  bool sped_up = false;
};
} // namespace ge
