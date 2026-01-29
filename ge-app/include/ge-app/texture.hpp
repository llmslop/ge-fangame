#pragma once

#include "ge-hal/gpu.hpp"
#include "ge-hal/surface.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace ge {

namespace detail {
template <usize N> struct uintN {};

template <> struct uintN<16> {
  using type = u16;
};
template <> struct uintN<32> {
  using type = u32;
};
} // namespace detail

template <PixelFormat format,
          class DType = typename detail::uintN<pixel_format_bpp(format)>::type>
class Texture : public ConstSurface {
public:
  Texture(const DType *color_data, u32 width, u32 height,
          PixelFormat ctor_format = format)
      : ConstSurface{color_data, width, width, height, format} {
    // Ensure format matches
    assert(ctor_format == format);
  }

  void blit(const Surface &region) {
    hal::gpu::blit_blend(region, *this, 0xFF);
  }

  void blit_blend(const Surface &region, u8 alpha) {
    hal::gpu::blit_blend(region, *this, alpha);
  }

  template <class Region>
  void blit_rotated(
      Region &region, int dst_cx,
      int dst_cy,      // destination center (screen coords)
      float angle_rad, // clockwise or CCW, your choice, just be consistent
      float src_cx = NAN, float src_cy = NAN) {
    const int W = region.get_width();
    const int H = region.get_height();

    // Precompute sin/cos
    float c = std::cos(angle_rad);
    float s = std::sin(angle_rad);

    // Sprite center in source space
    if (std::isnan(src_cx))
      src_cx = (get_width() - 1) * 0.5f;
    if (std::isnan(src_cy))
      src_cy = (get_height() - 1) * 0.5f;

    // Compute conservative bounding box radius
    float hw = get_width() * 0.5f;
    float hh = get_height() * 0.5f;
    float r = std::sqrt(hw * hw + hh * hh);

    int x0 = std::max(0, (int)std::floor(dst_cx - r));
    int x1 = std::min(W - 1, (int)std::ceil(dst_cx + r));
    int y0 = std::max(0, (int)std::floor(dst_cy - r));
    int y1 = std::min(H - 1, (int)std::ceil(dst_cy + r));

    // For each destination pixel, map back into source via inverse rotation
    // Inverse rotation matrix for [c -s; s c] is [c s; -s c]
    for (int y = y0; y <= y1; ++y) {
      float dy = (float)y - (float)dst_cy;
      for (int x = x0; x <= x1; ++x) {
        float dx = (float)x - (float)dst_cx;

        float u = c * dx + s * dy + src_cx;
        float v = -s * dx + c * dy + src_cy;

        int sx = (int)std::lround(u);
        int sy = (int)std::lround(v);

        if ((unsigned)sx < (unsigned)get_width() &&
            (unsigned)sy < (unsigned)get_height()) {
          auto col = *static_cast<const DType *>(pixel_at(sx, sy));
          if (!has_alpha_channel(format)) {
            region.set_pixel(x, y, col);
          } else if (format == PixelFormat::ARGB1555) {
            if (col & 0x8000U) {
              region.set_pixel(x, y, col);
            }
          } else if (format == PixelFormat::ARGB8888 &&
                     region.get_pixel_format() == PixelFormat::RGB565) {
            // for the time being we only support ARGB8888 -> RGB565 blit with
            // alpha
            auto src_color = region.get_pixel(x, y);
            u8 src_r = (src_color >> 11) & 0x1F;
            u8 src_g = (src_color >> 5) & 0x3F;
            u8 src_b = src_color & 0x1F;
            u8 dst_a = (col >> 24) & 0xFF;
            u8 dst_r = (col >> 16) & 0xFF;
            u8 dst_g = (col >> 8) & 0xFF;
            u8 dst_b = col & 0xFF;
            // Alpha blend
            u8 out_r = (dst_r * dst_a + (src_r << 3) * (255 - dst_a)) / 255;
            u8 out_g = (dst_g * dst_a + (src_g << 2) * (255 - dst_a)) / 255;
            u8 out_b = (dst_b * dst_a + (src_b << 3) * (255 - dst_a)) / 255;
            u16 out_color =
                ((out_r >> 3) << 11) | ((out_g >> 2) << 5) | (out_b >> 3);
            region.set_pixel(x, y, out_color);
          } else {
            std::printf("Unsupported format for blit_rotated with alpha\r\n");
            return;
          }
        }
      }
    }
  }
};

using TextureRGB565 = Texture<PixelFormat::RGB565>;
using TextureARGB1555 = Texture<PixelFormat::ARGB1555>;
using TextureARGB8888 = Texture<PixelFormat::ARGB8888>;

inline bool clip_blit_rect(i32 fb_w, i32 fb_h, i32 &dst_x, i32 &dst_y,
                           i32 &src_x, i32 &src_y, i32 &w, i32 &h) {
  // Clip left
  if (dst_x < 0) {
    i32 d = -dst_x;
    src_x += d;
    w -= d;
    dst_x = 0;
  }

  // Clip top
  if (dst_y < 0) {
    i32 d = -dst_y;
    src_y += d;
    h -= d;
    dst_y = 0;
  }

  // Clip right
  if (dst_x + w > fb_w)
    w = fb_w - dst_x;

  // Clip bottom
  if (dst_y + h > fb_h)
    h = fb_h - dst_y;

  return w > 0 && h > 0;
}

} // namespace ge
