#pragma once

#include "ge-hal/core.hpp"
#include "ge-hal/stm/gpio.hpp"
#include "stm32f429xx.h"

namespace ge {
namespace hal {
namespace stm {

// MAX98357A is a mono I2S DAC with integrated amplifier
// It only requires I2S signals: BCLK (bit clock), LRCK (word select), DIN
// (data) No separate chip select or enable pin needed (SD pin can be used for
// gain/shutdown)

// I2S configuration for audio output
struct I2SConfig {
  SPI_TypeDef *spi; // SPI peripheral used for I2S (SPI2 or SPI3)
  Pin ws;           // Word select (LRCK)
  Pin ck;           // Bit clock (BCLK)
  Pin sd;           // Serial data (DIN to MAX98357A)
  u8 af;            // Alternate function number
  DMA_Stream_TypeDef *dma_stream;
  u8 dma_channel;
  IRQn_Type dma_irqn;
};

// I2S handle for audio operations
struct I2SHandle {
  SPI_TypeDef *spi;
  DMA_Stream_TypeDef *dma_stream;

  // Initialize I2S peripheral for audio output
  // sample_rate: audio sample rate in Hz (e.g., 8000, 16000, 44100)
  void init(const I2SConfig &config, u32 sample_rate);

  // Start DMA transfer of audio samples
  // buffer: pointer to audio sample buffer (16-bit samples)
  // length: number of 16-bit samples
  void start_dma(const i16 *buffer, u32 length);

  // Stop DMA transfer
  void stop_dma();

  // Check if DMA is currently active
  bool is_dma_active() const;
};

// I2S configuration for SPI2 (commonly used for I2S audio)
// Using pins for STM32F429-Discovery that avoid USB HS conflicts:
// - PB12: I2S2_WS (Word Select / LRCK)
// - PB13: I2S2_CK (Bit Clock / BCLK)
// - PC3:  I2S2_SD (Serial Data / DIN) - avoids PB15 USB_HS_DP conflict
static const I2SConfig I2S2_CONFIG = {
    .spi = SPI2,
    .ws = Pin('B', 12),
    .ck = Pin('B', 13),
    .sd = Pin('C', 3),
    .af = 5, // AF5 for I2S2
    .dma_stream = DMA1_Stream4,
    .dma_channel = 0, // Channel 0 for SPI2_TX
    .dma_irqn = DMA1_Stream4_IRQn,
};

// Initialize I2S audio subsystem
void init_i2s_audio(u32 sample_rate);

// Get the global I2S handle
I2SHandle &get_i2s_handle();

} // namespace stm
} // namespace hal
} // namespace ge
