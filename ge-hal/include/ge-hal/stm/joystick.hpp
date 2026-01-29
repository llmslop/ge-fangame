#pragma once

#include "ge-hal/core.hpp"
#include "ge-hal/stm/gpio.hpp"
#include "ge-hal/stm/time.hpp"
#include "stm32f429xx.h" // Replace with your specific family header (e.g., stm32f10x.h)

namespace ge {
namespace hal {
namespace stm {

// ---------------------------------------------------------
// Global Buffer
// ---------------------------------------------------------
// [0] = PC4 (X-Axis), [1] = PC5 (Y-Axis)
// "volatile" prevents the compiler from optimizing reads away
volatile uint16_t joystick_data[2];

// ---------------------------------------------------------
// Function Prototypes
// ---------------------------------------------------------
void init_joystick_dma_adc(void);

void joystick_read(uint16_t *x, uint16_t *y) {
  *x = joystick_data[0];
  *y = joystick_data[1];
}

// ---------------------------------------------------------
// Initialization
// ---------------------------------------------------------
void init_joystick_dma_adc(void) {
  Pin ux{'C', 4}, uy{'C', 5};
  ux.set_mode(GPIOMode::Analog);
  ux.set_pupd(GPIOPuPd::NoPull);
  uy.set_mode(GPIOMode::Analog);
  uy.set_pupd(GPIOPuPd::NoPull);

  // DMA2 (Required for ADC1) is on AHB1
  RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
  // ADC1 is on APB2
  RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

  // --- Configure DMA2 Stream 0 ---

  // Disable the stream first to edit registers
  DMA2_Stream0->CR &= ~DMA_SxCR_EN;
  while (DMA2_Stream0->CR & DMA_SxCR_EN)
    ;

  DMA2_Stream0->PAR = (uint32_t)&(ADC1->DR);
  DMA2_Stream0->M0AR = (uint32_t)joystick_data;
  DMA2_Stream0->NDTR = 2;

  // Configure DMA Control Register (CR)
  DMA2_Stream0->CR = 0;                           // Reset
  DMA2_Stream0->CR |= (0U << DMA_SxCR_CHSEL_Pos); // Channel 0 selection
  DMA2_Stream0->CR |= 1U << DMA_SxCR_MSIZE_Pos;   // 16-bit
  DMA2_Stream0->CR |=
      (1U << DMA_SxCR_PSIZE_Pos); // PSIZE: Peripheral size = 16-bit (Half-word)
  DMA2_Stream0->CR |=
      DMA_SxCR_MINC; // Memory Increment (Write to [0], then [1])
  DMA2_Stream0->CR |=
      DMA_SxCR_CIRC; // Circular Mode (Loop back to [0] after [1])
  DMA2_Stream0->CR |= (0U << DMA_SxCR_DIR_Pos); // Peripheral to Memory
  DMA2_Stream0->CR |= DMA_SxCR_EN;

  // --- 4. Configure ADC1 ---

  // -- Sequence Setup --
  // L (Length) = 2 conversions. Register expects (Count - 1), so we write 1.
  // Bits 23:20 in SQR1
  ADC1->SQR1 &= ~ADC_SQR1_L_Msk;
  ADC1->SQR1 |= (1U << ADC_SQR1_L_Pos);

  // Assign Channels to "Slots"
  // Slot 1 = Channel 14 (PC4)
  // Slot 2 = Channel 15 (PC5)
  ADC1->SQR3 = (14U << 0) | (15U << 5);

  // Channels 14 & 15 are in SMPR1
  ADC1->SMPR1 |= (ADC_SMPR1_SMP14_2 | ADC_SMPR1_SMP14_1) | // 144 cycles
                 (ADC_SMPR1_SMP15_2 | ADC_SMPR1_SMP15_1);

  // Set ADC prescaler to /4 (01) or /6 (10) or /8 (11)
  // Note: ADC_CCR is a common register for all ADCs
  ADC->CCR &= ~ADC_CCR_ADCPRE;  // Clear
  ADC->CCR |= ADC_CCR_ADCPRE_0; // Set to /4 (Assuming APB2 > 60MHz)

  // -- Control Setup --
  // SCAN: Scan mode (convert entire sequence of channels)
  ADC1->CR1 |= ADC_CR1_SCAN;

  // CONT: Continuous mode (restart sequence immediately after finishing)
  ADC1->CR2 |= ADC_CR2_CONT;

  // DMA: Enable DMA requests
  ADC1->CR2 |= ADC_CR2_DMA;

  // DDS: DMA Disable Selection.
  // IMPORTANT: Set this to 1. It keeps DMA requests generating forever.
  // If 0, DMA stops after the first loop in some configs.
  ADC1->CR2 |= ADC_CR2_DDS;

  // -- Power On --
  ADC1->CR2 |= ADC_CR2_ADON;

  delay_spin(1000); // Wait for ADC to stabilize

  ADC1->CR2 |= ADC_CR2_SWSTART;
}

} // namespace stm
} // namespace hal
} // namespace ge
