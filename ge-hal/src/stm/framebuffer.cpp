#include "ge-hal/stm/framebuffer.hpp"

#include "ge-hal/app.hpp"
#include "ge-hal/stm/gpio.hpp"
#include "ge-hal/stm/sdram.hpp"
#include "ge-hal/stm/spi.hpp"
#include "ge-hal/stm/time.hpp"
#include "stm32f429xx.h"
#include <initializer_list>

namespace ge {
namespace hal {
namespace stm {

static GE_SDRAM u16 framebuffer_storage[2][ge::App::WIDTH * ge::App::HEIGHT];

u16 *pixel_buffer(int buffer_index) {
  return framebuffer_storage[buffer_index & 1];
}

struct LCD {
  Pin csx{'C', 2}, wrx{'D', 13};
  SPIHandle spi = nullptr;

  enum class Command : u8 {
    eRESET = 0x01,
    eSLEEP_OUT = 0x11,
    eGAMMA = 0x26,
    eDISPLAY_OFF = 0x28,
    eDISPLAY_ON = 0x29,
    eCOLUMN_ADDR = 0x2A,
    ePAGE_ADDR = 0x2B,
    eGRAM = 0x2C,
    eMAC = 0x36,
    ePIXEL_FORMAT = 0x3A,
    eWDB = 0x51,
    eWCD = 0x53,
    eRGB_INTERFACE = 0xB0,
    eFRC = 0xB1,
    eBPC = 0xB5,
    eDFC = 0xB6,
    ePOWER1 = 0xC0,
    ePOWER2 = 0xC1,
    eVCOM1 = 0xC5,
    eVCOM2 = 0xC7,
    ePOWERA = 0xCB,
    ePOWERB = 0xCF,
    ePGAMMA = 0xE0,
    eNGAMMA = 0xE1,
    eDTCA = 0xE8,
    eDTCB = 0xEA,
    ePOWER_SEQ = 0xED,
    e3GAMMA_EN = 0xF2,
    eINTERFACE = 0xF6,
    ePRC = 0xF7
  };

  void exec(Command cmd, std::initializer_list<u8> data = {}) {
    // select the current peripheral
    csx.write(false);

    // command mode
    wrx.write(false);
    spi.send_blocking(static_cast<u8>(cmd));

    // NOTE: without this, things dont work on Release
    while (spi.is_busy())
      delay_spin(1);

    // data mode
    if (data.size() > 0) {
      wrx.write(true);
      for (u8 d : data) {
        spi.send_blocking(d);
      }
    }

    // NOTE: without this, things dont work on Release
    while (spi.is_busy())
      delay_spin(1);
    // deselect the current peripheral
    csx.write(true);
  }

  LCD() {
    spi = SPI5_CONFIG.init();
    for (auto pin : {csx, wrx}) {
      pin.set_mode(GPIOMode::Output);
      pin.set_otype(GPIOOType::PushPull);
      pin.set_pupd(GPIOPuPd::NoPull);
      pin.set_speed(GPIOSpeed::Medium);
    }

    csx.write(true);
    initialization_sequence();
  }

  void initialization_sequence() {
    exec(Command::eRESET);
    delay_spin(50000);
    exec(Command::ePOWERA, {0x39, 0x2C, 0x00, 0x34, 0x02});
    exec(Command::ePOWERB, {0x00, 0xC1, 0x30});
    exec(Command::eDTCA, {0x85, 0x00, 0x78});
    exec(Command::eDTCB, {0x00, 0x00});
    exec(Command::ePOWER_SEQ, {0x64, 0x03, 0x12, 0x81});
    exec(Command::ePRC, {0x20});
    exec(Command::ePOWER1, {0x23});
    exec(Command::ePOWER2, {0x10});
    exec(Command::eVCOM1, {0x3E, 0x28});
    exec(Command::eVCOM2, {0x86});
    exec(Command::eMAC, {0x48});
    exec(Command::ePIXEL_FORMAT, {0x55});
    exec(Command::eFRC, {0x00, 0x18});
    exec(Command::eDFC, {0x08, 0x82, 0x27});
    exec(Command::e3GAMMA_EN, {0x00});
    exec(Command::eCOLUMN_ADDR, {0x00, 0x00, 0x00, 0xEF});
    exec(Command::ePAGE_ADDR, {0x00, 0x00, 0x01, 0x3F});
    exec(Command::eGAMMA, {0x01});

    exec(Command::ePGAMMA, {0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1,
                            0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00});
    exec(Command::eNGAMMA, {0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1,
                            0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F});
    exec(Command::eSLEEP_OUT);

    delay_spin(1000000); // Wait for wake up
    exec(Command::eDISPLAY_ON);
  }
};

void init_ltdc() {
  // clock setup
  RCC->APB1ENR |= RCC_APB1ENR_PWREN;
  RCC->CR &= ~RCC_CR_PLLSAION;

  static constexpr u32 PLLSAIN = 192, PLLSAIR = 4;
  RCC->PLLSAICFGR =
      (PLLSAIN << RCC_PLLSAICFGR_PLLSAIN_Pos) |
      (PLLSAIR << RCC_PLLSAICFGR_PLLSAIR_Pos); // set PLLSAI N and R
  RCC->DCKCFGR &= ~RCC_DCKCFGR_PLLSAIDIVR;     // clear DIVR
  // 00: /2, 01: /4, 10: /8, 11: /16.
  RCC->DCKCFGR |= (0x03UL << RCC_DCKCFGR_PLLSAIDIVR_Pos); // set DIVR to /16
  RCC->CR |= RCC_CR_PLLSAION;
  while (!(RCC->CR & RCC_CR_PLLSAIRDY))
    delay_spin(1); // Wait for lock

  // enable LTDC clock
  RCC->APB2ENR |= RCC_APB2ENR_LTDCEN;

  // config pins
  auto init_pins = [](char bank, std::initializer_list<u8> nums,
                      bool af9 = false) {
    for (auto num : nums) {
      Pin pin{bank, num};
      pin.set_mode(GPIOMode::AlternateFunction);
      pin.set_otype(GPIOOType::PushPull);
      pin.set_pupd(GPIOPuPd::NoPull);
      pin.set_speed(GPIOSpeed::High);
      pin.set_af(af9 ? 9 : 14);
    }
  };

  // LCD_Rx
  init_pins('C', {10});         // R2
  init_pins('B', {0, 1}, true); // R3, R6
  init_pins('A', {11, 12});     // R4, R5
  init_pins('G', {6});          // R7
  // LCD_Gx
  init_pins('A', {6});        // G2
  init_pins('G', {10}, true); // G3
  init_pins('B', {10, 11});   // G4, G5
  init_pins('C', {7});        // G6
  init_pins('D', {3});        // G7
  // LCD_Bx
  init_pins('D', {6});        // B2
  init_pins('G', {11});       // B3
  init_pins('G', {12}, true); // B4
  init_pins('A', {3});        // B5
  init_pins('B', {8, 9});     // B6, B7
  /// control pins
  init_pins('A', {4});  // VSYNC
  init_pins('C', {6});  // HSYNC
  init_pins('G', {7});  // CLK
  init_pins('F', {10}); // DE

  // Horizontal and Vertical timing setup
  u32 hsw = 10, hbp = 30, hfp = 1;
  u32 vsw = 2, vbp = 1, vfp = 3;
  LTDC->SSCR =
      ((hsw - 1) << LTDC_SSCR_HSW_Pos) | ((vsw - 1) << LTDC_SSCR_VSH_Pos);
  LTDC->BPCR = ((hsw + hbp - 1) << LTDC_BPCR_AHBP_Pos) |
               ((vsw + vbp - 1) << LTDC_BPCR_AVBP_Pos);
  LTDC->AWCR = ((hsw + hbp + App::WIDTH - 1) << LTDC_AWCR_AAW_Pos) |
               ((vsw + vbp + App::HEIGHT - 1) << LTDC_AWCR_AAH_Pos);
  LTDC->TWCR = ((hsw + hbp + App::WIDTH + hfp - 1) << LTDC_TWCR_TOTALW_Pos) |
               ((vsw + vbp + App::HEIGHT + vfp - 1) << LTDC_TWCR_TOTALH_Pos);
  LTDC->LIPCR = App::HEIGHT + vbp + vsw - 1;
  LTDC->IER |= LTDC_IER_LIE;
  NVIC_SetPriority(LTDC_IRQn, 0);
  NVIC_EnableIRQ(LTDC_IRQn);

  LTDC->GCR &=
      ~(LTDC_GCR_HSPOL | LTDC_GCR_VSPOL | LTDC_GCR_DEPOL | LTDC_GCR_PCPOL);

  // Layer1 setup
  LTDC_Layer1->WHPCR =
      ((hsw + hbp) << LTDC_LxWHPCR_WHSTPOS_Pos) |
      ((hsw + hbp + App::WIDTH - 1) << LTDC_LxWHPCR_WHSPPOS_Pos);
  LTDC_Layer1->WVPCR =
      ((vsw + vbp) << LTDC_LxWVPCR_WVSTPOS_Pos) |
      ((vsw + vbp + App::HEIGHT - 1) << LTDC_LxWVPCR_WVSPPOS_Pos);
  LTDC_Layer1->PFCR = 0x02UL << LTDC_LxPFCR_PF_Pos;
  LTDC_Layer1->CACR = 0xFFUL << LTDC_LxCACR_CONSTA_Pos;
  LTDC_Layer1->CFBAR = reinterpret_cast<u32>(framebuffer_storage[0]);
  u32 pitch = App::WIDTH * 2;
  u32 line_length = App::WIDTH * 2;
  LTDC_Layer1->CFBLR = (pitch << 16) | (line_length + 3);
  LTDC_Layer1->CFBLNR = App::HEIGHT << LTDC_LxCFBLNR_CFBLNBR_Pos;
  LTDC_Layer1->CR |= 1;

  LCD lcd;
  lcd.exec(LCD::Command::eRGB_INTERFACE, {0xC2});
  lcd.exec(LCD::Command::eINTERFACE, {0x01, 0x00, 0x06});
  lcd.exec(LCD::Command::ePIXEL_FORMAT, {0x55});

  // Enable Global LTDC
  LTDC->GCR |= LTDC_GCR_LTDCEN;
  LTDC->SRCR = LTDC_SRCR_IMR;

  lcd.csx.write(true);

  for (int i = 0; i < ge::App::WIDTH * ge::App::HEIGHT; i++) {
    framebuffer_storage[0][i] = 0x001F;
    framebuffer_storage[1][i] = 0xF800;
  }
}

volatile bool vblank = false;

bool begin_frame(u32 &buffer_index) {
  // 1. Basic check: Did the ISR fire?
  if (!vblank) {
    return false;
  }

  // 2. TIMING CHECK (Crucial for Audio/Polling)
  // Check if we are currently in the Active Video area.
  // VDES (Vertical Data Enable) is High when pixels are being drawn.
  if (LTDC->CDSR & LTDC_CDSR_VDES) {
    // We are LATE. The screen is already drawing the old buffer again.
    // If we wait now, we block for ~16ms.
    // Instead, we skip this frame entirely to keep the CPU free for
    // Audio/Logic.
    vblank = false;
    return false;
  }

  // 3. We are in the Safe Zone (VBlank). Commit the Swap.
  vblank = false;

  LTDC_Layer1->CFBAR = reinterpret_cast<u32>(pixel_buffer(buffer_index));

  // Since we verified we are in VBlank (via VDES check above),
  // we can use Immediate Reload (IMR) safely, or VBR.
  // VBR is the safest standard.
  LTDC->SRCR = LTDC_SRCR_VBR;

  // 4. Wait for hardware to accept the command
  // Since we are in VBlank, this clears almost instantly.
  while (LTDC->SRCR & LTDC_SRCR_VBR) {
    delay_spin(1);
  }

  // 5. Give app the new buffer
  buffer_index ^= 1;
  return true;
}

} // namespace stm
} // namespace hal
} // namespace ge

extern "C" void LTDC_IRQHandler() {
  using namespace ge::hal::stm;
  if (LTDC->ISR & LTDC_ISR_LIF) {
    // Clear the interrupt flag
    LTDC->ICR = LTDC_ICR_CLIF;

    // Signal the application
    vblank = true;
  }
}
