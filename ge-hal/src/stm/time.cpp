
#include "ge-hal/stm/time.hpp"
#include "stm32f429xx.h"

ge::u32 SystemCoreClock = ge::hal::stm::SYS_FREQUENCY;

static volatile ge::u32 systick_counter = 0;
extern "C" void SysTick_Handler() { ++systick_counter; }

namespace ge {
namespace hal {
namespace stm {

void delay_spin(volatile u32 count) {
  while (count--)
    __NOP();
}

void systick_init(u32 frequency) {
  RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN; // Enable SYSCFG
  SysTick_Config(SystemCoreClock / frequency);
}

u32 systick_get() { return systick_counter; }

void delay_timed(u32 time_units) {
  u32 start = systick_get();
  while ((systick_get() - start) < time_units)
    delay_spin(10);
}

void setup_clock() {
  RCC->PLLCFGR &= 0xFFFF0000UL;
  RCC->PLLCFGR |= (((PLL_P - 2) >> 1) & 0x3UL) << 16;
  RCC->PLLCFGR |= PLL_M | (PLL_N << 6);
  RCC->PLLCFGR |= (PLL_Q << 24);
  RCC->PLLCFGR |= RCC_PLLCFGR_PLLSRC_HSI;
  RCC->CR |= RCC_CR_PLLON;
  while ((RCC->CR & RCC_CR_PLLRDY) == 0)
    delay_spin(1);
  RCC->CFGR |= (APB1_PRE << 10) | (APB2_PRE << 13);
  RCC->CFGR |= RCC_CFGR_SW_PLL;
  while ((RCC->CFGR & RCC_CFGR_SWS) == 0)
    delay_spin(1);

  RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
  SysTick_Config(SystemCoreClock / 1000); // Tick every 1 ms
}

} // namespace stm
} // namespace hal
} // namespace ge
