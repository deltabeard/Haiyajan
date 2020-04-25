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

#include <SDL2/SDL.h>

#include <libretro.h>

#pragma once

/* Functions in this file are not thread safe. */

enum
{
	PLAY_LOG_CATEGORY_CORE = SDL_LOG_CATEGORY_CUSTOM,
};

/**
 * Initialise callback functions of libretro core.
 *
 * \param ctx	Libretro core context.
 */
void play_init_cb(struct core_ctx_s *ctx);

/**
 * Play a single frame of the libretro core.
 *
 * \param ctx	Libretro core context.
 */
void play_frame(struct core_ctx_s *ctx);

/**
 * Initialise the audio and video contexts for libretro core.
 *
 * \param ctx	Libretro core context.
 * \returns	0 on success, else failure. Use SDL_GetError().
 */
uint_fast8_t play_init_av(struct core_ctx_s *ctx);

/**
 * Free audio and video contexts for libretro core.
 *
 * \param ctx	Libretro core context.
 */
void play_deinit_cb(struct core_ctx_s *ctx);
