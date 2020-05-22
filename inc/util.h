/**
 * Various utility functions for Haiyajan.
 * Copyright (C) 2020  Mahyar Koshkouei
 *
 * This is free software, and you are welcome to redistribute it under the terms
 * of the GNU Affero General Public License version 3.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 *
 * See the LICENSE file for more details.
 */

#pragma once

#include <SDL2/SDL.h>

void gen_filename(char filename[static 64], const char *core_name,
		  const char fmt[static 3]);

/**
 * Modify the atomic to a value after a given time.
 */
void set_atomic_timeout(Uint32 timeout_ms, SDL_atomic_t *atomic, int setval,
	const char *name);

/**
 * Makes sure that delayed threads no longer attempt to modify data that may
 * now be out of scope.
 */
void util_exit_all(void);

typedef struct util_cond_s util_cond;
