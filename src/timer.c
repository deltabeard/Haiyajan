#include <SDL2/SDL.h>

#include <timer.h>

void timer_init(struct timer_ctx_s *const tim, double targ_rate)
{
	tim->display_ms = (1 / targ_rate) * 1000.0;
	tim->timer_fract = 0.0;
	tim->timer_accumulator = 0;

	return;
}

int timer_show_frame(struct timer_ctx_s *const tim, Uint32 elapsed_ms)
{
	double delay = tim->display_ms - (double) elapsed_ms;
	double delay_ms = SDL_floor(delay);

	/* FIXME: What happens if the delay is negative? */
	tim->timer_fract += delay - delay_ms;

	if(tim->timer_fract <= -(tim->display_ms))
		tim->timer_fract += tim->display_ms;
	else if(tim->timer_fract > 1.0)
	{
		double acc_int = SDL_floor(tim->timer_fract);
		delay_ms += acc_int;
		tim->timer_fract -= acc_int;
	}

	tim->timer_accumulator += (int)delay_ms;

	return (int)tim->timer_accumulator;
}
