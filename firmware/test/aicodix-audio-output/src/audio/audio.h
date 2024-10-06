#pragma once

#include "I2SOutput.h"
#include "ES8388.h"
#include "BufferSync.h"

bool audio_init(BufferSync *dac_sample_generator, const uint32_t sampleRate);