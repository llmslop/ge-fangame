#include "ge-app/game/sky.hpp"
#include "assets/out/textures/moon.h"
#include "assets/out/textures/sun.h"

namespace ge {

// ============================================================
// Sky color helpers (private to this TU)
// ============================================================
namespace {

constexpr u16 SKY_NIGHT_RGB = 0x0821;
constexpr u16 SKY_DAY_RGB = 0x5DBF;
constexpr u16 SKY_WARM_RGB = 0xF2C0;

inline float clamp01(float x) { return std::max(0.0f, std::min(1.0f, x)); }

inline float smooth01(float x) {
  x = clamp01(x);
  return x * x * (3.0f - 2.0f * x);
}

inline u16 blend_rgb565(uint16_t a, uint16_t b, float t) {
  t = clamp01(t);
  u8 ti = static_cast<u8>(t * 255.0f);
  return ge::blend_rgb565(a, b, ti);
}

} // namespace

inline u16 sky_color(float t) {
  t = std::fmod(t, 1.0f);
  if (t < 0)
    t += 1.0f;

  constexpr float SUNRISE_START = 0.23f;
  constexpr float SUNRISE_END = 0.27f;
  constexpr float SUNSET_START = 0.73f;
  constexpr float SUNSET_END = 0.77f;

  if (t >= SUNRISE_START && t < SUNRISE_END) {
    float k = smooth01((t - SUNRISE_START) / (SUNRISE_END - SUNRISE_START));
    return (k < 0.5f)
               ? blend_rgb565(SKY_NIGHT_RGB, SKY_WARM_RGB, k * 2.0f)
               : blend_rgb565(SKY_WARM_RGB, SKY_DAY_RGB, (k - 0.5f) * 2.0f);
  }

  if (t >= SUNSET_START && t < SUNSET_END) {
    float k = smooth01((t - SUNSET_START) / (SUNSET_END - SUNSET_START));
    return (k < 0.5f)
               ? blend_rgb565(SKY_DAY_RGB, SKY_WARM_RGB, k * 2.0f)
               : blend_rgb565(SKY_WARM_RGB, SKY_NIGHT_RGB, (k - 0.5f) * 2.0f);
  }

  if (t >= SUNRISE_END && t < SUNSET_START)
    return SKY_DAY_RGB;

  return SKY_NIGHT_RGB;
}

inline u8 luminance565(u16 c) {
  u8 r = (c >> 11) & 0x1F;
  u8 g = (c >> 5) & 0x3F;
  u8 b = c & 0x1F;

  // scale to ~8-bit domain
  r <<= 3;
  g <<= 2;
  b <<= 3;

  // perceptual-ish
  return (r * 77 + g * 150 + b * 29) >> 8;
}

inline u16 gray565(u8 lum) {
  u16 r = (lum >> 3) & 0x1F; // 5 bits
  u16 g = (lum >> 2) & 0x3F; // 6 bits
  u16 b = (lum >> 3) & 0x1F;
  return (r << 11) | (g << 5) | b;
}

inline u16 cloud_from_sky(u16 sky, float t) {
  // t in [0,1)
  // reuse your day cycle

  constexpr float SUNRISE_START = 0.23f;
  constexpr float SUNRISE_END = 0.27f;
  constexpr float SUNSET_START = 0.73f;
  constexpr float SUNSET_END = 0.77f;
  constexpr u16 CLOUD_BASE_RGB = 0xEF7B;

  float day_k;
  if (t < SUNRISE_START || t > SUNSET_END) {
    day_k = 0.0f; // night
  } else if (t < SUNRISE_END) {
    day_k = smooth01((t - SUNRISE_START) / (SUNRISE_END - SUNRISE_START));
  } else if (t < SUNSET_START) {
    day_k = 1.0f; // full day
  } else {
    day_k = 1.0f - smooth01((t - SUNSET_START) / (SUNSET_END - SUNSET_START));
  }

  // 1) tint clouds toward sky hue (but don't lose identity)
  u16 c = ge::blend_rgb565(CLOUD_BASE_RGB, sky,
                           u8(48 + 48 * day_k) // more sky tint during day
  );

  // 2) darken at night, slightly at day
  u8 night_dark = u8(96 * (1.0f - day_k)); // never full black
  c = ge::blend_rgb565(c, 0x0000, night_dark);

  return c;
}

Sky::Sky()
    : sun_texture{sun, sun_WIDTH, sun_HEIGHT, sun_FORMAT_CPP},
      moon_texture{moon, moon_WIDTH, moon_HEIGHT, moon_FORMAT_CPP} {}

u8 Sky::luminance_at_time(float t) {
  u16 sc = sky_color(t);
  return luminance565(sc);
}

void Sky::render(App &app, Surface render_region, Clock &clock) {
  auto x_offset = clock.get_game_timer().get(app) / 1000 % CLOUD_TEXTURE_WIDTH;
  auto sky_color = ::ge::sky_color(clock.time_in_day(app));
  auto cloud_color = cloud_from_sky(sky_color, clock.time_in_day(app));

  const int W = render_region.get_width();
  const int H = render_region.get_height();
  const int TEX_W = CLOUD_TEXTURE_WIDTH;
  auto fb = render_region;

  // Fill sky upper region
  hal::gpu::fill(render_region.subsurface(0, 0, W, H - CLOUD_TEXTURE_HEIGHT),
                 sky_color);

  // temporary: recalculate cloud LUT every frame
  for (usize i = 0; i < sizeof(CLOUD_COLORS) / sizeof(CLOUD_COLORS[0]); ++i) {
    cloud_lut[i] = blend_rgb565(sky_color, cloud_color, CLOUD_COLORS[i]);
  }

  assert(H == 80);
  int stride = bg_clouds_len / H;

  auto sun = render_celestial_object(sun_texture, render_region,
                                     clock.time_in_day(app), sky_color);
  bool sun_visible = (sun.w > 0 && sun.h > 0);
  auto moon = render_celestial_object(
      moon_texture, render_region,
      std::fmod(clock.time_in_day(app) + 0.5f, 1.0f), sky_color);
  bool moon_visible = (moon.w > 0 && moon.h > 0);

  for (i32 dy = 0; dy < CLOUD_TEXTURE_HEIGHT; ++dy) {
    const u8 *row_rle = &bg_clouds[CLOUD_ROW_OFFSETS[dy]];
    i32 tex_x = 0;

    auto y = dy + H - CLOUD_TEXTURE_HEIGHT;
    const bool sun_row = sun_visible && (y >= sun.y && y < sun.y + sun.h);

    while (true) {
      u8 elem = *row_rle++;
      i32 len = elem >> 4;
      i32 value = elem & 0x0F;
      len = (len < 0xF) ? (len + 1) : *row_rle++;

      i32 run_start = tex_x;
      tex_x += len;

      // map texture X → screen X with wrap
      i32 sx = run_start - x_offset;
      if (sx < 0)
        sx += TEX_W;

      auto draw_span = [&](i32 dx, i32 span_len) {
        if (span_len <= 0 || dx >= W)
          return;

        span_len = std::min(span_len, W - dx);

        auto overlaps = [&](const Sky::Rect &r) {
          return r.w > 0 && y >= r.y && y < r.y + r.h &&
                 !(dx + span_len <= r.x || dx >= r.x + r.w);
        };

        bool sun_hit = sun_visible && overlaps(sun);
        bool moon_hit = moon_visible && overlaps(moon);

        // fast path: no celestial overlap at all
        if (!sun_hit && !moon_hit) {
          hal::gpu::fill(fb.subsurface(dx, y, span_len, 1), cloud_lut[value]);
          return;
        }

        // slow path: per-pixel blend where needed
        for (i32 x = dx; x < dx + span_len; ++x) {
          u16 out = cloud_lut[value];

          // sun already drawn into framebuffer
          if (sun_visible && x >= sun.x && x < sun.x + sun.w && y >= sun.y &&
              y < sun.y + sun.h) {
            u16 bg = fb.get_pixel(x, y);
            out = blend_rgb565(bg, cloud_color, CLOUD_COLORS[value]);
          }

          // moon already drawn into framebuffer
          if (moon_visible && x >= moon.x && x < moon.x + moon.w &&
              y >= moon.y && y < moon.y + moon.h) {
            u16 bg = fb.get_pixel(x, y);
            out = blend_rgb565(bg, cloud_color, CLOUD_COLORS[value]);
          }

          fb.set_pixel(x, y, out);
        }
      };

      // normal span
      if (sx < W)
        draw_span(sx, len);

      // wrapped span
      if (sx + len > TEX_W)
        draw_span(0, (sx + len) - TEX_W);

      if (tex_x >= TEX_W)
        break;
    }
  }
}

Sky::Rect Sky::render_celestial_object(const TextureARGB8888 &texture,
                                       Surface fb, float t, u16 sky_color) {
  constexpr float SUNRISE = 0.25f;
  constexpr float SUNSET = 0.75f;

  if (t < SUNRISE || t > SUNSET)
    return {0, 0, 0, 0};

  float day_t = clamp01((t - SUNRISE) / (SUNSET - SUNRISE));

  const i32 W = fb.get_width();
  const i32 H = fb.get_height();

  const i32 SUN_W = texture.get_width();
  const i32 SUN_H = texture.get_height();

  // --- center-based position ---
  i32 cx = i32(day_t * (W + SUN_W)) - SUN_W / 2;
  i32 cy = i32((H - 12) - std::sin(day_t * M_PI) * (H - 22));

  // --- top-left ---
  i32 dst_x = cx - SUN_W / 2;
  i32 dst_y = cy - SUN_H / 2;

  i32 src_x = 0;
  i32 src_y = 0;
  i32 draw_w = SUN_W;
  i32 draw_h = SUN_H;

  if (!clip_blit_rect(W, H, dst_x, dst_y, src_x, src_y, draw_w, draw_h))
    return {0, 0, 0, 0};

  auto src =
      texture.subsurface(u32(src_x), u32(src_y), u32(draw_w), u32(draw_h));

  auto dst = fb.subsurface(u32(dst_x), u32(dst_y), u32(draw_w), u32(draw_h));

  hal::gpu::fill(dst, sky_color);
  hal::gpu::blit_blend(dst, src, 0xFF);

  return {dst_x, dst_y, draw_w, draw_h};
}
} // namespace ge
