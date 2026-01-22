#pragma once

#include "ge-hal/surface.hpp"
#include <cstdint>
#include <cstdio>

namespace ge {

struct GlyphContext {
  char ch;
  int gx, gy; // glyph-local pixel (render space, includes scale)
  int gw, gh; // glyph dimensions in render space
  int x, y;   // glyph origin in layout space (unscaled)
};

// TODO: currently only supporting Proggy (mono 8x8 font)
// In the future, we might ship our own font instead of relying on external
// ones.
// This API can also support variable-width fonts in the future to save space.
class Font {
public:
  Font(const u8 *glyph_data, u8 glyph_width, u8 glyph_height, char first_char,
       char last_char, u8 glyph_byte_per_row, const u8 *glyph_advances)
      : glyph_data(glyph_data), glyph_width(glyph_width),
        glyph_height(glyph_height), first_char(first_char),
        last_char(last_char), glyph_bytes_per_row(glyph_byte_per_row),
        glyph_advances(glyph_advances) {}

  static const Font &regular_font();
  static const Font &bold_font();
  static const Font &big_font();

  bool get_glyph(char c, u8 const *&glyph_data, u8 &glyph_w, u8 &glyph_h,
                 u8 &advance) const;

  u32 line_height() const;
  u32 default_advance() const;

  template <class ColorCallback>
  void render(const char *text, u32 max_len, Surface region, int x0, int y0,
              ColorCallback cb) const {
    int x = x0, y = y0;
    const int max_x = region.get_width(), max_y = region.get_height();

    auto measure_word = [&](const char *p) {
      int w = 0;
      for (; *p && *p != ' ' && *p != '\n'; ++p) {
        u8 const *gd;
        u8 gw, gh, adv;
        if (get_glyph(*p, gd, gw, gh, adv))
          w += adv;
        else
          w += default_advance();
      }
      return w;
    };

    for (auto ch = *text; ch && max_len; ch = *++text, --max_len) {

      // Hard stop if we're vertically out of bounds
      if (y + line_height() > max_y)
        break;

      // Explicit newline
      if (ch == '\n') {
        x = x0;
        y += line_height();
        continue;
      }

      // Word wrap on space
      if (ch == ' ') {
        int space_adv = default_advance();
        int word_w = measure_word(text + 1);

        if (x + space_adv + word_w > max_x) {
          x = x0;
          y += line_height();
          continue; // skip leading space
        }
      }

      u8 const *glyph_data;
      u8 glyph_w, glyph_h, advance;
      bool has_glyph = get_glyph(ch, glyph_data, glyph_w, glyph_h, advance);

      if (!has_glyph)
        advance = default_advance();

      // Character-level wrap fallback (long words)
      if (x + advance > max_x) {
        x = x0;
        y += line_height();
      }

      if (has_glyph) {
        for (int gy = 0; gy < glyph_h; ++gy) {
          for (int gx = 0; gx < glyph_w; ++gx) {
            int bit = gy * glyph_w + gx;
            bool pixel_on = (glyph_data[bit >> 3] >> (7 - (bit & 7))) & 1;

            if (pixel_on) {
              auto color = cb(GlyphContext{ch, gx, gy, glyph_w, glyph_h, x, y});
              region.set_pixel(x + gx, y + gy, color);
            }
          }
        }
      }

      x += advance;
    }
  }

  void render_colored(const char *text, u32 max_len, Surface region, int x,
                      int y, std::uint16_t color) const {
    render(text, max_len, region, x, y,
           [color](const GlyphContext &) { return color; });
  }

  u32 text_width(const char *text, u32 max_len) const {
    u32 width = 0;
    for (auto ch = *text; ch && max_len; ch = *++text, --max_len) {
      u8 const *glyph_data;
      u8 glyph_w, glyph_h, advance;
      bool has_glyph = get_glyph(ch, glyph_data, glyph_w, glyph_h, advance);
      if (!has_glyph)
        advance = default_advance();
      width += advance;
    }
    return width;
  }

private:
  const u8 *glyph_data, *glyph_advances;
  u8 glyph_width, glyph_height, glyph_bytes_per_row;
  char first_char, last_char;
};
} // namespace ge
