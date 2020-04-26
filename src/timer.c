#include <SDL2/SDL.h>

#include <timer.h>

void timer_init(struct timer_ctx_s *const tim, double targ_rate)
{
	tim->display_ms = (1 / targ_rate) * 1000.0;
	tim->timer_accumulator = 0.0;

	return;
}

int timer_get_delay(struct timer_ctx_s *const tim, Uint32 elapsed_ms)
{
	double delay = tim->display_ms - (double) elapsed_ms;
	double delay_ms = SDL_floor(delay);

	/* FIXME: What happens if the delay is negative? */
	tim->timer_accumulator += delay - delay_ms;

	if(tim->timer_accumulator <= -(tim->display_ms))
	{
		tim->timer_accumulator += tim->display_ms;
	}
	else if(tim->timer_accumulator > 1.0)
	{
		double acc_int = SDL_floor(tim->timer_accumulator);
		delay_ms += acc_int;
		tim->timer_accumulator -= acc_int;
	}

	return (int)delay_ms;
}
