#pragma once

#include <SDL2/SDL.h>

struct timer_ctx_s
{
	double display_ms;
	double timer_fract;
	int timer_accumulator;
};

/**
 * Initialises the timer context and sets the display refresh rate.
 */
void timer_init(struct timer_ctx_s *const tim, double targ_rate);

/**
 * Returns weather the current frame should be shown or not. A frame may be
 * skipped if the content refresh rate is faster than the display refresh rate.
 * If the content is running too fast, this function will execute an internal
 * delay in order to compensate.
 *
 * \returns	Negative for skip frame, 0 for no delay, else time to delay for..
 */
int timer_show_frame(struct timer_ctx_s *const tim, Uint32 elapsed_ms);
