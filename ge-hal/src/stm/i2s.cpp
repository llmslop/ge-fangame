#include "ge-hal/stm/i2s.hpp"
#include "ge-hal/stm/audio.hpp"
#include "ge-hal/stm/time.hpp"

namespace ge {
namespace hal {
namespace stm {

namespace {
I2SHandle g_i2s_handle;
} // namespace

void I2SHandle::init(const I2SConfig &config, u32 sample_rate) {
  spi = config.spi;
  dma_stream = config.dma_stream;

  // Enable GPIO clocks and configure pins for I2S alternate function
  for (Pin pin : {config.ws, config.ck, config.sd}) {
    pin.set_mode(GPIOMode::AlternateFunction);
    pin.set_otype(GPIOOType::PushPull);
    pin.set_pupd(GPIOPuPd::NoPull);
    pin.set_speed(GPIOSpeed::VeryHigh);
    pin.set_af(config.af);
  }

  // Enable SPI2 clock (on APB1)
  RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;

  // Enable DMA1 clock
  RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;

  // Disable SPI/I2S before configuration
  spi->I2SCFGR = 0;
  spi->CR1 = 0;

  // Configure I2S
  // I2SCFGR register:
  // - I2SMOD = 1: I2S mode selected
  // - I2SCFG = 10: Master transmit
  // - I2SSTD = 00: I2S Philips standard
  // - DATLEN = 00: 16-bit data
  // - CHLEN = 0: 16-bit per channel

  u32 i2scfgr = SPI_I2SCFGR_I2SMOD |       // I2S mode
                (0b10 << SPI_I2SCFGR_I2SCFG_Pos); // Master transmit

  // Calculate I2S clock divider for desired sample rate
  // I2S clock source is PLLI2S or I2S_CKIN
  // For now, use PLLI2S with default configuration
  // The actual sample rate depends on the I2S clock and divider settings

  // Enable PLLI2S
  // Configure PLLI2S: PLLI2SN=192, PLLI2SR=5 for 48kHz-compatible rates
  RCC->PLLI2SCFGR = (192 << RCC_PLLI2SCFGR_PLLI2SN_Pos) |
                    (5 << RCC_PLLI2SCFGR_PLLI2SR_Pos);
  RCC->CR |= RCC_CR_PLLI2SON;
  while (!(RCC->CR & RCC_CR_PLLI2SRDY)) {
    delay_spin(1);
  }

  // Calculate I2SDIV and ODD for the desired sample rate
  // PLLI2SCLK = HSI/PLL_M * PLLI2SN / PLLI2SR = 16/8 * 192 / 5 = 76.8 MHz
  // I2S_CLK = PLLI2SCLK / (2 * I2SDIV + ODD)
  // For 8000 Hz with 16-bit stereo: need MCLK = 256 * Fs = 2.048 MHz
  // I2SDIV = PLLI2SCLK / (256 * Fs) / 2

  u32 plli2sclk = (16 * 192 / 8 / 5) * 1000000; // 76.8 MHz
  u32 target_mclk = 256 * sample_rate;
  u32 div = plli2sclk / target_mclk;
  u8 i2sdiv = div / 2;
  u8 odd = div & 1;

  // Clamp divider to valid range
  if (i2sdiv < 2)
    i2sdiv = 2;
  if (i2sdiv > 255)
    i2sdiv = 255;

  // Configure I2S prescaler register
  spi->I2SPR = (odd << SPI_I2SPR_ODD_Pos) | i2sdiv | SPI_I2SPR_MCKOE;

  // Apply I2S configuration
  spi->I2SCFGR = i2scfgr;

  // Configure DMA for I2S TX
  // DMA1 Stream4, Channel 0 for SPI2_TX
  dma_stream->CR = 0;
  while (dma_stream->CR & DMA_SxCR_EN) {
    delay_spin(1);
  }

  // Clear all DMA interrupt flags for Stream4
  DMA1->HIFCR = DMA_HIFCR_CTCIF4 | DMA_HIFCR_CHTIF4 | DMA_HIFCR_CTEIF4 |
               DMA_HIFCR_CDMEIF4 | DMA_HIFCR_CFEIF4;

  // DMA configuration:
  // - Channel 0
  // - Memory to peripheral
  // - Circular mode
  // - Memory increment, peripheral fixed
  // - 16-bit memory and peripheral size
  // - High priority
  // - Half-transfer and transfer-complete interrupts
  dma_stream->CR = (config.dma_channel << DMA_SxCR_CHSEL_Pos) |
                   DMA_SxCR_DIR_0 |       // Memory to peripheral
                   DMA_SxCR_CIRC |        // Circular mode
                   DMA_SxCR_MINC |        // Memory increment
                   DMA_SxCR_MSIZE_0 |     // 16-bit memory size
                   DMA_SxCR_PSIZE_0 |     // 16-bit peripheral size
                   DMA_SxCR_PL_1 |        // High priority
                   DMA_SxCR_HTIE |        // Half-transfer interrupt
                   DMA_SxCR_TCIE;         // Transfer-complete interrupt

  dma_stream->PAR = reinterpret_cast<u32>(&spi->DR);

  // Enable DMA interrupt in NVIC
  NVIC_SetPriority(config.dma_irqn, 5);
  NVIC_EnableIRQ(config.dma_irqn);

  // Enable I2S
  spi->I2SCFGR |= SPI_I2SCFGR_I2SE;

  // Enable SPI DMA TX request
  spi->CR2 |= SPI_CR2_TXDMAEN;
}

void I2SHandle::start_dma(const i16 *buffer, u32 length) {
  // Disable DMA stream before configuration
  dma_stream->CR &= ~DMA_SxCR_EN;
  while (dma_stream->CR & DMA_SxCR_EN) {
    delay_spin(1);
  }

  // Clear DMA interrupt flags
  DMA1->HIFCR = DMA_HIFCR_CTCIF4 | DMA_HIFCR_CHTIF4 | DMA_HIFCR_CTEIF4 |
               DMA_HIFCR_CDMEIF4 | DMA_HIFCR_CFEIF4;

  // Configure DMA transfer
  dma_stream->M0AR = reinterpret_cast<u32>(buffer);
  dma_stream->NDTR = length;

  // Enable DMA stream
  dma_stream->CR |= DMA_SxCR_EN;
}

void I2SHandle::stop_dma() {
  // Disable DMA stream
  dma_stream->CR &= ~DMA_SxCR_EN;
  while (dma_stream->CR & DMA_SxCR_EN) {
    delay_spin(1);
  }
}

bool I2SHandle::is_dma_active() const {
  return (dma_stream->CR & DMA_SxCR_EN) != 0;
}

void init_i2s_audio(u32 sample_rate) {
  g_i2s_handle.init(I2S2_CONFIG, sample_rate);
}

I2SHandle &get_i2s_handle() { return g_i2s_handle; }

} // namespace stm
} // namespace hal
} // namespace ge

// DMA1 Stream4 interrupt handler for I2S audio
extern "C" void DMA1_Stream4_IRQHandler() {
  using namespace ge::hal::stm;

  // Check for half-transfer or transfer-complete
  if (DMA1->HISR & (DMA_HISR_HTIF4 | DMA_HISR_TCIF4)) {
    // Clear interrupt flags
    DMA1->HIFCR = DMA_HIFCR_CHTIF4 | DMA_HIFCR_CTCIF4;

    // Fill the next buffer half
    audio_engine_fill_buffer();
  }

  // Clear any error flags
  if (DMA1->HISR & (DMA_HISR_TEIF4 | DMA_HISR_DMEIF4 | DMA_HISR_FEIF4)) {
    DMA1->HIFCR = DMA_HIFCR_CTEIF4 | DMA_HIFCR_CDMEIF4 | DMA_HIFCR_CFEIF4;
  }
}
