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

#include <util.h>
#include <timer.h>

int timer_init(struct timer_ctx_s *const tim, double emulated_rate)
{
	int ret = 0;
	tim->core_ms = (1.0 / emulated_rate) * 1000.0;
	tim->core_us = ((1.0 / emulated_rate) * 1000.0 * 1024.0);
	tim->timer_accumulator = 0.0;
	tim->delay_comp_ms = ((int)tim->core_ms + 1) * 2;
	tim->timer_event = SDL_RegisterEvents(1);
	tim->busy_acu_ms = 0;
	tim->busy_samples = 0;

	if(tim->timer_event == (Uint32) -1)
		ret = -1;

	return ret;
}

void timer_profile_start(struct timer_ctx_s *const tim)
{
	tim->profile_start_ms = SDL_GetTicks();
	return;
}

void timer_profile_end(struct timer_ctx_s *const tim)
{
	enum timer_status_e status;
	tim->busy_acu_ms += SDL_GetTicks() - tim->profile_start_ms;
	tim->busy_samples++;

	if(tim->busy_samples < 32)
		return;

	Uint64 busy_acu_us = tim->busy_acu_ms * 1024;
	busy_acu_us /= 32;
	tim->busy_acu_ms = 0;
	tim->busy_samples = 0;

	if(busy_acu_us >= tim->core_us * 1.5)
		status = TIMER_SPEED_UP_AGGRESSIVELY;
	else if(busy_acu_us >= (tim->core_us / 1.5))
		status = TIMER_SPEED_UP;
	else
		status = TIMER_OKAY;

	SDL_LogVerbose(SDL_LOG_CATEGORY_SYSTEM, "Average busy time %.2f",
		       ((float)busy_acu_us / 1024));

	{
		SDL_Event event;
		SDL_zero(event);
		event.type = tim->timer_event;
		event.user.code = status;
		SDL_PushEvent(&event);
	}
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
