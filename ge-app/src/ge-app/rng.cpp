#include "ge-app/rng.hpp"

#ifdef GE_HAL_PC
#include <random>
#else
#ifdef GE_HAL_STM32
#include "ge-hal/stm/rng.hpp"
#endif
#endif

namespace ge {
PCG32 &PCG32::instance() {
  static PCG32 instance;
  return instance;
}

namespace rng {
void init_seed() {
  u32 seed = 0;
#ifdef GE_HAL_PC
  seed = std::random_device{}(); // get OS RNG seed
#else
  hal::stm::init_rng();
  seed = hal::stm::rng_read();
#endif
}
} // namespace rng
} // namespace ge
