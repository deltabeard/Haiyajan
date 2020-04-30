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

#include <SDL2/SDL.h>

#include <timer.h>

int timer_init(struct timer_ctx_s *const tim, double emulated_rate)
{
	tim->core_ms = (1.0 / emulated_rate) * 1000.0;
	tim->timer_accumulator = 0.0;
	tim->delay_comp_ms = ((int)tim->core_ms + 1) * 2;

	return 0;
}

int timer_get_delay(struct timer_ctx_s *const tim, Uint32 elapsed_ms)
{
	tim->timer_accumulator -= (double)elapsed_ms;
	tim->timer_accumulator += tim->core_ms;

	/* Render the next frame immediately without waiting for VSYNC. */
	if(tim->timer_accumulator < -tim->core_ms)
		return -1;
	/* Do not render a new frame for the next VSYNC call. */
	else if(tim->timer_accumulator > tim->core_ms)
		return tim->delay_comp_ms;

	/* Play the next frame on the next VSYNC call as normal. */
	return 0;
}
