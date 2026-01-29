#pragma once

#include "ge-app/game/sky.hpp"
#include "ge-hal/app.hpp"
#include "ge-hal/gpu.hpp"

namespace ge {
class Dock {
public:
  void render(App &app, Surface &surface, Clock &clock, i32 boat_x,
              i32 boat_y) {
    // Dock rectangle: (-infty, -infty) -> (infty, -40)
    i32 dock_top = surface.get_height() - 40 + boat_y;
    if (dock_top >= surface.get_height())
      return;

    auto dock_region = surface.subsurface(0, dock_top, surface.get_width(),
                                          surface.get_height() - dock_top);
    auto sky_lum = Sky::luminance_at_time(clock.time_in_day(app));
    auto color = blend_rgb565(0x0000, 0x8B45, sky_lum);
    hal::gpu::fill(dock_region, color); // brown
  }

  static u16 map_color() {
    return 0x8B45; // brown
  }
};
} // namespace ge
