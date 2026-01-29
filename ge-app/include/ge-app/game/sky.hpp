#pragma once

#include "assets/out/textures/clouds.h"
#include "ge-app/game/clock.hpp"
#include "ge-app/gfx/color.hpp"
#include "ge-app/texture.hpp"
#include "ge-hal/app.hpp"
#include "ge-hal/surface.hpp"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <utility>

namespace ge {
class Sky {
public:
  Sky();

  static int max_x_offset() { return CLOUD_TEXTURE_WIDTH; }
  static u8 luminance_at_time(float t);

  void render(App &app, Surface render_region, Clock &clock);

private:
  TextureARGB8888 sun_texture, moon_texture;
  u16 cloud_lut[sizeof(CLOUD_COLORS) / sizeof(CLOUD_COLORS[0])];

  struct Rect {
    i32 x, y, w, h;
  };

  Rect render_celestial_object(const TextureARGB8888 &texture,
                               Surface render_region, float t, u16 sky_color);
};
} // namespace ge
