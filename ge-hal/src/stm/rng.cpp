#include "ge-hal/stm/rng.hpp"

#include "ge-hal/stm/time.hpp"
#include "stm32f429xx.h"
#include <cstdio>

namespace ge {
namespace hal {
namespace stm {

void init_rng() {
  // Enable RNG clock
  RCC->AHB2ENR |= RCC_AHB2ENR_RNGEN;
  // Enable RNG
  RNG->CR |= RNG_CR_RNGEN;
}

u32 rng_read() {
  // Wait until data is ready
  while (!(RNG->SR & RNG_SR_DRDY))
    delay_spin(1);

  u32 value = RNG->DR;

  if (RNG->SR & (RNG_SR_CECS | RNG_SR_SECS)) {
    // Error occurred, clear flags
    std::printf("RNG error: SR=0x%08X\r\n", RNG->SR);
    RNG->SR &= ~(RNG_SR_CECS | RNG_SR_SECS);
    return 0xDEADBEEF; // idk
  }
  // Read and return random number
  return value;
}

} // namespace stm
} // namespace hal
} // namespace ge
