#pragma once

#include "ge-hal/gpu.hpp"
#include "ge-hal/surface.hpp"

namespace ge {
inline void draw_rect(Surface &surface, u16 color, u32 stroke_width = 2) {
  hal::gpu::fill(surface.subsurface(0, 0, surface.get_width(), stroke_width),
                 color);
  hal::gpu::fill(surface.subsurface(0, surface.get_height() - stroke_width,
                                    surface.get_width(), stroke_width),
                 color);
  hal::gpu::fill(surface.subsurface(0, 0, stroke_width, surface.get_height()),
                 color);
  hal::gpu::fill(surface.subsurface(surface.get_width() - stroke_width, 0,
                                    stroke_width, surface.get_height()),
                 color);
}
} // namespace ge
