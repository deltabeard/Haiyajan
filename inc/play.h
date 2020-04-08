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

/**
 * Initialise callback functions of libretro core.
 *
 * \param c	Libretro core context.
 */
void play_init_cb(struct core_ctx_s *c);

/**
 * Play a single frame of the libretro core.
 */
void play_frame(void);

/**
 * Initialise the audio and video contexts for libretro core.
 *
 * \returns 0 on success, else failure. Use SDL_GetError().
 */
uint_fast8_t play_init_av(void);

/**
 * Free audio and video contexts for libretro core.
 */
void play_deinit_display(void);
