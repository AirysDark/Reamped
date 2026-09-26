#include "AudioOutput.h"

#include "driver/i2s.h"
#include "ReampedPins.h"

bool AudioOutput::begin() {
  i2s_config_t cfg = {};
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  cfg.sample_rate = REAMPED_SAMPLE_RATE;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  cfg.dma_buf_count = 8;
  cfg.dma_buf_len = 128;
  cfg.use_apll = false;
  cfg.tx_desc_auto_clear = true;
  cfg.fixed_mclk = REAMPED_MCLK_HZ;

  i2s_pin_config_t pins = {};
  pins.mck_io_num = REAMPED_PIN_MCLK;
  pins.bck_io_num = REAMPED_PIN_BCLK;
  pins.ws_io_num = REAMPED_PIN_LRCLK;
  pins.data_out_num = REAMPED_PIN_SDOUT;
  pins.data_in_num = I2S_PIN_NO_CHANGE;

  esp_err_t err = i2s_driver_install((i2s_port_t)REAMPED_I2S_PORT, &cfg, 0, nullptr);
  if (err != ESP_OK) {
    Serial.printf("[I2S] driver install failed: %d\n", (int)err);
    return false;
  }

  err = i2s_set_pin((i2s_port_t)REAMPED_I2S_PORT, &pins);
  if (err != ESP_OK) {
    Serial.printf("[I2S] pin setup failed: %d\n", (int)err);
    return false;
  }

  err = i2s_set_clk(
    (i2s_port_t)REAMPED_I2S_PORT,
    REAMPED_SAMPLE_RATE,
    I2S_BITS_PER_SAMPLE_16BIT,
    I2S_CHANNEL_STEREO
  );
  if (err != ESP_OK) {
    Serial.printf("[I2S] clock setup failed: %d\n", (int)err);
    return false;
  }

  i2s_zero_dma_buffer((i2s_port_t)REAMPED_I2S_PORT);
  started_ = true;

  // Prime clocks and DMA with silence.
  writeSilence(32);

  Serial.println("[I2S] 48 kHz, stereo, 16-bit I2S, MCLK=12.288 MHz");
  return true;
}

bool AudioOutput::write(const int16_t *samples, size_t sampleCount) {
  if (!started_ || samples == nullptr || sampleCount == 0) {
    return false;
  }

  size_t written = 0;
  const size_t bytes = sampleCount * sizeof(int16_t);

  const esp_err_t err = i2s_write(
    (i2s_port_t)REAMPED_I2S_PORT,
    samples,
    bytes,
    &written,
    portMAX_DELAY
  );

  return err == ESP_OK && written == bytes;
}

void AudioOutput::writeSilence(size_t frames) {
  constexpr size_t kChunkFrames = 64;
  int16_t silence[kChunkFrames * 2] = {};

  while (frames > 0) {
    const size_t chunk = frames > kChunkFrames ? kChunkFrames : frames;
    write(silence, chunk * 2);
    frames -= chunk;
  }
}
