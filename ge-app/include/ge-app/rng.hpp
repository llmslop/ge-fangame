#pragma once

#include "ge-hal/core.hpp"
namespace ge {
struct PCG32 {
  using result_type = u32;

  u64 state = 0x853c49e6748fea9bULL;
  u64 inc = 0xda3e39cb94b95bdbULL;

  static constexpr result_type min() { return 0; }
  static constexpr result_type max() { return UINT32_MAX; }

  result_type operator()() {
    u64 old = state;
    state = old * 6364136223846793005ULL + (inc | 1);
    u32 xorshifted = ((old >> 18u) ^ old) >> 27u;
    u32 rot = old >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
  }

  static PCG32 &instance();
};

namespace rng {
void init_seed();

inline u32 next() { return PCG32::instance()(); }

// Generate a float in [0.0, 1.0), only 24 bits of precision
inline f32 next_float() {
  auto r = next() >> 8;
  return r * (1.0f / 16777216.0f);
}
} // namespace rng
} // namespace ge
