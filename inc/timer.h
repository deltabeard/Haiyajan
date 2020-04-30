/**
 * Handles framerate timing.
 * Copyright (C) 2020  Mahyar Koshkouei
 *
 * This is free software, and you are welcome to redistribute it under the terms
 * of the GNU Affero General Public License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 *
 * See the LICENSE file for more details.
 */

#pragma once

#include <SDL2/SDL.h>

struct timer_ctx_s
{
	double core_ms;
	double timer_accumulator;
	int delay_comp_ms;
};

/* TODO: update docs. */
/* TODO: use Uint64 instead of double. */

/**
 * Initialises the timer context and sets the display refresh rate.
 */
int timer_init(struct timer_ctx_s *const tim, double emulated_rate);

/**
 * Returns weather the current frame should be shown or not. A frame may be
 * skipped if the content refresh rate is faster than the display refresh rate.
 * If the content is running too fast, this function will execute an internal
 * delay in order to compensate.
 *
 * \returns	Negative for skip frame, 0 for no delay, else time to delay for.
 */
int timer_get_delay(struct timer_ctx_s *const tim, Uint32 elapsed_ms);
