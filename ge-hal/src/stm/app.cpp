#include "ge-hal/app.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "ge-hal/stm/dma2d.hpp"
#include "ge-hal/stm/framebuffer.hpp"
#include "ge-hal/stm/gpio.hpp"
#include "ge-hal/stm/joystick.hpp"
#include "ge-hal/stm/sdram.hpp"
#include "ge-hal/stm/time.hpp"
#include "stm32f429xx.h"
#include <ge-hal/stm/uart.hpp>

ge::hal::stm::UARTHandle stdout_usart = nullptr;

namespace ge {

namespace {
// Button GPIO pins (to be configured based on actual hardware)
constexpr hal::stm::Pin BUTTON1_PIN{'A', 0};
constexpr hal::stm::Pin BUTTON2_PIN{'C', 13};
constexpr int NUM_BUTTONS = 2;

// Button state tracking for event detection
constexpr i64 BUTTON_HOLD_THRESHOLD_MS = 1000;
struct ButtonState {
  bool last_state = false; // false = not pressed, true = pressed
  i64 last_down = -1;
  i64 last_up = -1;
  bool handled_hold = false;
} button_states[NUM_BUTTONS];

constexpr hal::stm::Pin button_pins[NUM_BUTTONS] = {BUTTON1_PIN, BUTTON2_PIN};

// Forward declaration for interrupt access
App *app_instance = nullptr;
} // anonymous namespace

// Handle button state change (called from interrupt)
void handle_button_interrupt(int button_index) {
  bool pressed = !button_pins[button_index].read();
  auto &bs = button_states[button_index];

  // Detect button down event
  if (pressed && !bs.last_state) {
    bs.last_down = app_instance->now();
    bs.last_up = -1;
    bs.handled_hold = false;
  }

  // Detect button up event
  if (!pressed && bs.last_state) {
    bs.last_up = app_instance->now();
    if (bs.last_down >= 0) {
      i64 held_time = bs.last_up - bs.last_down;
      if (held_time < BUTTON_HOLD_THRESHOLD_MS) {
        app_instance->on_button_clicked(static_cast<Button>(button_index));
      } else {
        app_instance->on_button_finished_hold(
            static_cast<Button>(button_index));
      }
    }
    bs.handled_hold = false;
  }

  bs.last_state = pressed;
}

static void enable_fpu() {
  SCB->CPACR |=
      (3UL << 20) | (3UL << 22); // enable FPU: CP10 and CP11 full access
}

static void config_flash() {
  FLASH->ACR |= FLASH_ACR_ICEN | FLASH_ACR_PRFTEN |
                FLASH_ACR_LATENCY_5WS; // enable cache, set latency
}

void App::system_init() {
  enable_fpu();
  config_flash();
  hal::stm::setup_clock();
  RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

  NVIC_SetPriority(EXTI0_IRQn, 5);
  NVIC_EnableIRQ(EXTI0_IRQn);

  NVIC_SetPriority(EXTI15_10_IRQn, 5);
  NVIC_EnableIRQ(EXTI15_10_IRQn);
}

App::App() {
  app_instance = this;

  stdout_usart = hal::stm::USART_CONFIG_DEBUG.init(115200);
  hal::stm::init_sdram();
  hal::stm::init_ltdc();
  hal::stm::init_dma2d();
  hal::stm::init_joystick_dma_adc();

  // Initialize button GPIO pins as inputs with pull-up resistors and enable
  // interrupts
  for (const auto &pin : button_pins) {
    pin.set_mode(hal::stm::GPIOMode::Input);
    pin.set_pupd(hal::stm::GPIOPuPd::PullUp);
    // Enable interrupt on both edges (press and release)
    pin.enable_exti(hal::stm::EXTITrigger::RisingFalling);
  }
}

App::~App() = default;

App::operator bool() { return true; }

static u32 buffer_index = 0;

std::int64_t App::now() { return hal::stm::systick_get(); }

JoystickState App::get_joystick_state() {
  constexpr int JOY_MIN = 0;
  constexpr int JOY_MAX = 4095;
  constexpr int JOY_CENTER_X = 2453;
  constexpr int JOY_CENTER_Y = 2112;
  constexpr int DEADZONE = 100;

  u16 x_raw = 0, y_raw = 0;
  hal::stm::joystick_read(&x_raw, &y_raw);

  auto normalize = [](u16 val, int center) -> float {
    int delta = static_cast<int>(val) - center;
    if (std::abs(delta) < DEADZONE) {
      return 0.0f;
    }
    if (delta > 0) {
      return static_cast<float>(delta) / static_cast<float>(JOY_MAX - center);
    } else {
      return static_cast<float>(delta) / static_cast<float>(center - JOY_MIN);
    }
  };

  // TODO: implement joystick reading
  return {-normalize(x_raw, JOY_CENTER_X), normalize(y_raw, JOY_CENTER_Y)};
}

void App::log(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  std::vprintf(fmt, args);
  std::fputs("\r\n", stdout);
  std::fflush(stdout);
  va_end(args);
}

void App::sleep(std::int64_t ms) { hal::stm::delay_timed(ms); }

void App::tick(float dt) {
  // Only check for hold events (press/release are handled by interrupts)
  for (int i = 0; i < NUM_BUTTONS; ++i) {
    auto &bs = button_states[i];

    // Check for hold event
    if (bs.last_state && bs.last_down >= 0 && !bs.handled_hold) {
      i64 held_time = now() - bs.last_down;
      if (held_time >= BUTTON_HOLD_THRESHOLD_MS) {
        on_button_held(static_cast<Button>(i));
        bs.handled_hold = true;
      }
    }
  }
}

void App::loop() {
  i64 last_tick = now();
  while (*this) {
    i64 current = now();
    float dt = (current - last_tick) * 1e-3f;
    tick(dt);
    last_tick = current;

    // Check if vblank occurred and we should render this frame
    if (hal::stm::begin_frame(buffer_index)) {
      auto buffer = hal::stm::pixel_buffer(buffer_index);
      Surface fb_region{buffer,      App::WIDTH,          App::WIDTH,
                        App::HEIGHT, PixelFormat::RGB565, buffer_index};
      render(fb_region);
    }

    // TODO: Process audio when needed
    // if (needs_audio_processing) process_audio();

    // Wait for interrupt to save power
    __WFI();
  }
}

void App::audio_bgm_play(const std::uint8_t *data, std::size_t len, bool loop) {
  (void)data;
  (void)len;
  (void)loop;
}

void App::audio_bgm_stop() {}

bool App::audio_bgm_is_playing() { return false; }

void App::audio_sfx_play(const std::uint8_t *data, std::size_t len,
                         std::size_t rate) {
  (void)data;
  (void)len;
  (void)rate;
}

void App::audio_sfx_stop_all() {}

void App::audio_set_master_volume(std::uint8_t vol) { (void)vol; }

void App::request_quit() {
  // On STM32, the app runs indefinitely; this is a no-op
}

} // namespace ge

// EXTI interrupt handlers for button pins
extern "C" {

// EXTI0 interrupt handler (Button 1 on PA0)
void EXTI0_IRQHandler() {
  if (EXTI->PR & (1U << 0)) {
    EXTI->PR = (1U << 0); // Clear pending bit
    ge::handle_button_interrupt(0);
  }
}

// EXTI1 interrupt handler (Button 2 on PC13)
void EXTI15_10_IRQHandler() {
  // Check if Line 13 triggered (1 << 13)
  if (EXTI->PR & (1U << 13)) {
    EXTI->PR = (1U << 13);          // Clear pending
    ge::handle_button_interrupt(1); // Call logic for button 2
  }
}
}
