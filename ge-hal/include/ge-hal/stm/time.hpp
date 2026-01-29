#pragma once

#include "ge-hal/core.hpp"
namespace ge {
namespace hal {
namespace stm {

static constexpr u32 PLL_HSI = 16, PLL_M = 8, PLL_N = 180, PLL_P = 2, PLL_Q = 7;
static constexpr u32 APB1_PRE = 5, APB2_PRE = 4;
static constexpr u32 SYS_FREQUENCY =
    ((PLL_HSI * PLL_N / PLL_M / PLL_P) * 1000000);
static constexpr u32 APB2_FREQUENCY = SYS_FREQUENCY >> (APB2_PRE - 3);
static constexpr u32 APB1_FREQUENCY = SYS_FREQUENCY >> (APB1_PRE - 3);

void delay_spin(volatile u32 count);

void systick_init(u32 frequency);
u32 systick_get();
extern "C" void systick_handler();

void delay_timed(u32 ms);

void setup_clock();

} // namespace stm
} // namespace hal
} // namespace ge
