#pragma once

#include <SDL2/SDL.h>

struct timer_ctx_s
{
	double display_rate;
	double timer_accumulator;
	Uint32 count_prev;
};

/**
 * Initialises the timer context and sets the display refresh rate.
 */
void timer_init(struct timer_ctx_s *const tim, double display_rate);

void timer_pause(struct timer_ctx_s *const tim);

void timer_continue(struct timer_ctx_s *const tim);

/**
 * Returns weather the current frame should be shown or not. A frame may be
 * skipped if the content refresh rate is faster than the display refresh rate.
 * If the content is running too fast, this function will execute an internal
 * delay in order to compensate.
 *
 * \returns	0 for skip frame, else show frame.
 */
unsigned char timer_show_frame(struct timer_ctx_s *const tim);
