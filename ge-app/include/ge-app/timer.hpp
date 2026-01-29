#pragma once

#include "ge-hal/app.hpp"
namespace ge {

class Timer {
public:
  Timer() = default;

  i64 get_raw(App &app) const { return app.now(); }

  void set_multiplier(App &app, u32 num, u32 den) {
    assert(den != 0);

    i64 now = get_raw(app);
    accum += (now - last_state_change) * multiplier_num / multiplier_den;
    last_state_change = now;

    multiplier_num = num;
    multiplier_den = den;
  }

  i64 get(App &app) const {
    i64 now = get_raw(app);
    return accum + (now - last_state_change) * multiplier_num / multiplier_den;
  }

  u32 get_multiplier_num() const { return multiplier_num; }
  u32 get_multiplier_den() const { return multiplier_den; }

  void reset(i64 base_time = 0) {
    multiplier_num = 1;
    multiplier_den = 1;
    accum = base_time;
    last_state_change = 0;
  }

private:
  u32 multiplier_num = 1;
  u32 multiplier_den = 1;
  i64 accum = 0, last_state_change = 0;
};

} // namespace ge
