#pragma once

#include "ge-hal/core.hpp"
namespace ge {
namespace hal {
namespace stm {
void init_rng();
u32 rng_read();
} // namespace stm
} // namespace hal
} // namespace ge
