#pragma once

#include "ge-hal/core.hpp"
#include <cstddef>

namespace ge {
namespace hal {
namespace stm {

// Audio engine for STM32 with MAX98357A amplifier
// Manages BGM and SFX playback with software mixing

// Audio configuration
static constexpr u32 AUDIO_SAMPLE_RATE = 8000;
static constexpr u32 AUDIO_BUFFER_SAMPLES = 256; // Samples per DMA buffer half
static constexpr int MAX_SFX_CHANNELS = 4;

// Audio stream state
struct AudioStream {
  const u8 *data;
  std::size_t length;
  std::size_t pos;
  bool loop;
  bool active;
};

// Audio engine state
struct AudioEngine {
  // Double buffer for DMA (ping-pong buffering)
  i16 buffer[AUDIO_BUFFER_SAMPLES * 2];

  // BGM stream
  AudioStream bgm;

  // SFX channels
  AudioStream sfx[MAX_SFX_CHANNELS];

  // Master volume (0-255)
  u8 master_volume;

  // Current buffer half being filled (0 or 1)
  volatile u8 filling_half;

  // Flag indicating audio is active
  volatile bool active;
};

// Initialize the audio engine
void audio_engine_init();

// Start audio playback
void audio_engine_start();

// Stop audio playback
void audio_engine_stop();

// Play background music
// data: unsigned 8-bit PCM audio data
// length: number of samples
// loop: whether to loop the music
void audio_engine_bgm_play(const u8 *data, std::size_t length, bool loop);

// Stop background music
void audio_engine_bgm_stop();

// Check if BGM is playing
bool audio_engine_bgm_is_playing();

// Play sound effect
// data: unsigned 8-bit PCM audio data
// length: number of samples
// sample_rate: sample rate of the SFX (for rate conversion if needed)
void audio_engine_sfx_play(const u8 *data, std::size_t length,
                           std::size_t sample_rate);

// Stop all sound effects
void audio_engine_sfx_stop_all();

// Set master volume (0-255)
void audio_engine_set_volume(u8 vol);

// Fill the next buffer half with mixed audio
// Called from DMA interrupt when a buffer half is complete
void audio_engine_fill_buffer();

// Get the audio engine instance
AudioEngine &get_audio_engine();

} // namespace stm
} // namespace hal
} // namespace ge
