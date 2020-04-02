/**
 * Libretro player.
 * Copyright (C) 2020  Mahyar Koshkouei
 *
 * This is free software, and you are welcome to redistribute it under the terms
 * of the GNU General Public License version 3.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 *
 * See the LICENSE file for more details.
 */

#include <libretro.h>

#pragma once

/* Functions in this file are not threadsafe. */

void play_set_ctx(struct core_ctx_s *c);
void play_frame(void);

/**
 * Libretro environment callbacks.
 */
bool cb_retro_environment(unsigned cmd, void *data);

void cb_retro_video_refresh(const void *data, unsigned width, unsigned height,
			    size_t pitch);

void cb_retro_audio_sample(int16_t left, int16_t right);

size_t cb_retro_audio_sample_batch(const int16_t *data, size_t frames);

void cb_retro_input_poll(void);

int16_t cb_retro_input_state(unsigned port, unsigned device, unsigned index,
			     unsigned id);
