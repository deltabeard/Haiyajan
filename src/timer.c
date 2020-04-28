#include <SDL2/SDL.h>

#include <timer.h>

int timer_init(struct timer_ctx_s *const tim, double emulated_rate)
{
	tim->core_ms = (1.0 / emulated_rate) * 1000.0;
	tim->timer_accumulator = 0.0;

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
		return (int)tim->core_ms;

	/* Play the next frame on the next VSYNC call as normal. */
	return 0;
}
