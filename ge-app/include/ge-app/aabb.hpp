#pragma once

#include "ge-hal/core.hpp"
#include <algorithm>
#include <cstdint>

namespace ge {

struct AABB {
  i32 x;
  i32 y;
  i32 w;
  i32 h;

  constexpr AABB() : x(0), y(0), w(0), h(0) {}
  constexpr AABB(i32 x_, i32 y_, i32 w_, i32 h_) : x(x_), y(y_), w(w_), h(h_) {}

  // --- edges ---
  constexpr i32 left() const { return x; }
  constexpr i32 right() const { return x + w; }
  constexpr i32 top() const { return y; }
  constexpr i32 bottom() const { return y + h; }

  // --- centers ---
  constexpr i32 center_x() const { return x + w / 2; }
  constexpr i32 center_y() const { return y + h / 2; }

  // --- validity ---
  constexpr bool empty() const { return w <= 0 || h <= 0; }

  // --- containment ---
  constexpr bool contains(i32 px, i32 py) const {
    return px >= left() && px < right() && py >= top() && py < bottom();
  }

  constexpr bool contains(const AABB &o) const {
    return o.left() >= left() && o.right() <= right() && o.top() >= top() &&
           o.bottom() <= bottom();
  }

  // --- intersection ---
  constexpr bool intersects(const AABB &o) const {
    return !(o.right() <= left() || o.left() >= right() ||
             o.bottom() <= top() || o.top() >= bottom());
  }

  constexpr AABB intersection(const AABB &o) const {
    i32 nx = std::max(left(), o.left());
    i32 ny = std::max(top(), o.top());
    i32 nr = std::min(right(), o.right());
    i32 nb = std::min(bottom(), o.bottom());

    i32 nw = nr - nx;
    i32 nh = nb - ny;

    if (nw <= 0 || nh <= 0)
      return AABB{0, 0, 0, 0};

    return AABB{nx, ny, nw, nh};
  }

  // --- movement ---
  constexpr AABB translated(i32 dx, i32 dy) const {
    return AABB{x + dx, y + dy, w, h};
  }

  constexpr AABB expanded(i32 amount) const {
    return AABB{x - amount, y - amount, w + amount * 2, h + amount * 2};
  }
};

} // namespace ge
