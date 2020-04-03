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

void play_init_cb(struct core_ctx_s *c);
void play_frame(void);
uint_fast8_t play_init_av(void);
void play_deinit_display(void);
