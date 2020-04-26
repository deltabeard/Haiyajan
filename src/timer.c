#include <SDL2/SDL.h>

#include <timer.h>

void timer_init(struct timer_ctx_s *const tim, double display_rate)
{
	tim->display_rate = display_rate;
	tim->timer_accumulator = 0.0;
	tim->count_prev = SDL_GetTicks();

	return;
}

void timer_pause(struct timer_ctx_s *const tim)
{
	(void) tim;
	return;
}

void timer_continue(struct timer_ctx_s *const tim)
{
	(void) tim;
	return;
}

unsigned char timer_show_frame(struct timer_ctx_s *const tim)
{
	static unsigned frame = 0;
	(void) tim;

	frame++;

	if(frame % 11 == 0)
		return 0;

	return 1;
}
