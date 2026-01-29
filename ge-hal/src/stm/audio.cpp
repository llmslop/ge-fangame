#include "ge-hal/stm/audio.hpp"
#include "ge-hal/stm/i2s.hpp"

namespace ge {
namespace hal {
namespace stm {

namespace {
AudioEngine g_audio_engine;
} // namespace

void audio_engine_init() {
  // Clear audio engine state
  g_audio_engine.bgm = {};
  for (int i = 0; i < MAX_SFX_CHANNELS; ++i) {
    g_audio_engine.sfx[i] = {};
  }
  g_audio_engine.master_volume = 255;
  g_audio_engine.filling_half = 0;
  g_audio_engine.active = false;

  // Initialize I2S peripheral
  init_i2s_audio(AUDIO_SAMPLE_RATE);

  // Pre-fill both buffer halves with silence
  for (u32 i = 0; i < AUDIO_BUFFER_SAMPLES * 2; ++i) {
    g_audio_engine.buffer[i] = 0;
  }
}

void audio_engine_start() {
  if (g_audio_engine.active) {
    return;
  }

  g_audio_engine.active = true;
  g_audio_engine.filling_half = 0;

  // Start DMA with the full double buffer
  get_i2s_handle().start_dma(g_audio_engine.buffer, AUDIO_BUFFER_SAMPLES * 2);
}

void audio_engine_stop() {
  g_audio_engine.active = false;
  get_i2s_handle().stop_dma();
}

void audio_engine_bgm_play(const u8 *data, std::size_t length, bool loop) {
  g_audio_engine.bgm.data = data;
  g_audio_engine.bgm.length = length;
  g_audio_engine.bgm.pos = 0;
  g_audio_engine.bgm.loop = loop;
  g_audio_engine.bgm.active = true;

  // Start audio if not already running
  if (!g_audio_engine.active) {
    audio_engine_start();
  }
}

void audio_engine_bgm_stop() { g_audio_engine.bgm.active = false; }

bool audio_engine_bgm_is_playing() { return g_audio_engine.bgm.active; }

void audio_engine_sfx_play(const u8 *data, std::size_t length,
                           std::size_t sample_rate) {
  (void)sample_rate; // Rate conversion not implemented, assume same rate

  // Find an available SFX channel
  for (int i = 0; i < MAX_SFX_CHANNELS; ++i) {
    if (!g_audio_engine.sfx[i].active) {
      g_audio_engine.sfx[i].data = data;
      g_audio_engine.sfx[i].length = length;
      g_audio_engine.sfx[i].pos = 0;
      g_audio_engine.sfx[i].loop = false;
      g_audio_engine.sfx[i].active = true;

      // Start audio if not already running
      if (!g_audio_engine.active) {
        audio_engine_start();
      }
      return;
    }
  }

  // Voice steal: overwrite the first channel
  g_audio_engine.sfx[0].data = data;
  g_audio_engine.sfx[0].length = length;
  g_audio_engine.sfx[0].pos = 0;
  g_audio_engine.sfx[0].loop = false;
  g_audio_engine.sfx[0].active = true;

  if (!g_audio_engine.active) {
    audio_engine_start();
  }
}

void audio_engine_sfx_stop_all() {
  for (int i = 0; i < MAX_SFX_CHANNELS; ++i) {
    g_audio_engine.sfx[i].active = false;
  }
}

void audio_engine_set_volume(u8 vol) { g_audio_engine.master_volume = vol; }

void audio_engine_fill_buffer() {
  if (!g_audio_engine.active) {
    return;
  }

  // Determine which half to fill (opposite of currently playing)
  // DMA circular mode: HTIF means first half done, TCIF means second half done
  // After HTIF: fill first half (while second half plays)
  // After TCIF: fill second half (while first half plays)
  u8 half = g_audio_engine.filling_half;
  g_audio_engine.filling_half = 1 - half;

  i16 *buf = g_audio_engine.buffer + half * AUDIO_BUFFER_SAMPLES;
  u8 vol = g_audio_engine.master_volume;

  for (u32 i = 0; i < AUDIO_BUFFER_SAMPLES; ++i) {
    i32 mixed = 0;

    // Mix BGM
    AudioStream &bgm = g_audio_engine.bgm;
    if (bgm.active) {
      if (bgm.pos >= bgm.length) {
        if (bgm.loop) {
          bgm.pos = 0;
        } else {
          bgm.active = false;
        }
      }
      if (bgm.active) {
        // Convert unsigned 8-bit to signed
        mixed += static_cast<i32>(bgm.data[bgm.pos++]) - 128;
      }
    }

    // Mix SFX channels
    for (int c = 0; c < MAX_SFX_CHANNELS; ++c) {
      AudioStream &sfx = g_audio_engine.sfx[c];
      if (!sfx.active) {
        continue;
      }
      if (sfx.pos >= sfx.length) {
        sfx.active = false;
        continue;
      }
      // Convert unsigned 8-bit to signed
      mixed += static_cast<i32>(sfx.data[sfx.pos++]) - 128;
    }

    // Apply master volume
    mixed = (mixed * vol) / 255;

    // Clamp to signed 8-bit range, then scale to 16-bit for I2S
    if (mixed < -128)
      mixed = -128;
    if (mixed > 127)
      mixed = 127;

    // Scale 8-bit sample to 16-bit for I2S output
    buf[i] = static_cast<i16>(mixed << 8);
  }
}

AudioEngine &get_audio_engine() { return g_audio_engine; }

} // namespace stm
} // namespace hal
} // namespace ge
