#include "ge-hal/stm/dma2d.hpp"
#include "ge-hal/gpu.hpp"
#include "ge-hal/stm/time.hpp"
#include "ge-hal/surface.hpp"
#include <algorithm>

namespace ge {
namespace hal {

namespace stm {
void init_dma2d() { RCC->AHB1ENR |= RCC_AHB1ENR_DMA2DEN; }
} // namespace stm

namespace gpu {
void wait_idle() {
  while (DMA2D->CR & DMA2D_CR_START)
    stm::delay_spin(1);
}

enum class Mode : u8 {
  R2M = 0x3,
  M2M = 0x0,
  M2M_PFC = 0x1,
  M2M_BLEND = 0x2,
};

static void set_mode(Mode mode) {
  auto cr = DMA2D->CR;
  cr &= ~DMA2D_CR_MODE_Msk;
  cr = static_cast<u32>(mode) << DMA2D_CR_MODE_Pos;
  DMA2D->CR = cr;
}

static void setup_output(Surface dst) {
  DMA2D->OPFCCR = static_cast<u32>(dst.get_pixel_format());
  DMA2D->OMAR = reinterpret_cast<u32>(dst.data());
  DMA2D->OOR = dst.get_stride() - dst.get_width();
  DMA2D->NLR = (dst.get_width() << DMA2D_NLR_PL_Pos) |
               (dst.get_height() << DMA2D_NLR_NL_Pos);
}

static void setup_background(Surface bg) {
  DMA2D->BGPFCCR = static_cast<u32>(bg.get_pixel_format());
  DMA2D->BGMAR = reinterpret_cast<u32>(bg.data());
  DMA2D->BGOR = bg.get_stride() - bg.get_width();
}

static void setup_input(ConstSurface src, u8 global_alpha = 0,
                        u8 alpha_mode = 0) {
  uint32_t current_clut =
      DMA2D->FGPFCCR &
      (DMA2D_FGPFCCR_CCM | DMA2D_FGPFCCR_CS | DMA2D_FGPFCCR_START);
  DMA2D->FGPFCCR =
      current_clut |
      (static_cast<u32>(src.get_pixel_format()) << DMA2D_FGPFCCR_CM_Pos) |
      (alpha_mode << DMA2D_FGPFCCR_AM_Pos) |
      (global_alpha << DMA2D_FGPFCCR_ALPHA_Pos);
  DMA2D->FGMAR = reinterpret_cast<u32>(src.data());
  DMA2D->FGOR = src.get_stride() - src.get_width();
}

static void fire() { DMA2D->CR |= DMA2D_CR_START; }

void fill(Surface dst, u32 color) {
  wait_idle();
  set_mode(Mode::R2M);
  DMA2D->OCOLR = color;
  setup_output(dst);
  fire();
}

static void normalize_regions(Surface &dst, ConstSurface &src) {
  u32 width = std::min(src.get_width(), dst.get_width());
  u32 height = std::min(src.get_height(), dst.get_height());
  src = src.subsurface(0, 0, width, height);
  dst = dst.subsurface(0, 0, width, height);
}

void blit(Surface dst, ConstSurface src) {
  normalize_regions(dst, src);
  wait_idle();
  set_mode((src.get_pixel_format() == dst.get_pixel_format()) ? Mode::M2M
                                                              : Mode::M2M_PFC);
  setup_output(dst);
  setup_input(src);
  fire();
}

void blit_blend(Surface dst, ConstSurface src, u8 global_alpha) {
  normalize_regions(dst, src);
  wait_idle();
  set_mode(Mode::M2M_BLEND);
  setup_output(dst);
  setup_input(src, global_alpha, 2);
  setup_background(dst);
  fire();
}

void load_palette(const u32 *colors, usize num_colors) {
  wait_idle();

  // 1. Point to Colors
  DMA2D->FGCMAR = reinterpret_cast<u32>(colors);

  // 2. Configure Load
  // Size = count - 1. Mode = ARGB8888 (0).
  DMA2D->FGPFCCR = ((num_colors - 1) << DMA2D_FGPFCCR_CS_Pos) |
                   (0x00 << DMA2D_FGPFCCR_CCM_Pos);

  // 3. Start Load
  DMA2D->FGPFCCR |= DMA2D_FGPFCCR_START;

  // 4. Wait strictly for CLUT load to finish
  while (!(DMA2D->ISR & DMA2D_ISR_CTCIF))
    stm::delay_spin(1);
  DMA2D->IFCR = DMA2D_IFCR_CCTCIF;
}

void blit_indexed(Surface dst, ConstSurface src) {
  normalize_regions(dst, src);
  wait_idle();
  set_mode(Mode::M2M_PFC);
  setup_output(dst);
  setup_input(src);
  fire();
}

} // namespace gpu
} // namespace hal
} // namespace ge
