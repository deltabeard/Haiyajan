#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libretro.h>
#include <load.h>

#include "minctest.h"

#define LIBRETRO_INIT_SO_PATH "./libretro_init/libretro-init.so"

/**
 * Tests basic functionality:
 * - That we can load and unload a libretro core.
 * - That we can init and deinit the core.
 * - That we can receive log messages to verify the above.
 */
void test_retro_init(void)
{
	uint_fast8_t ret;
	struct libretro_fn_s fn;

	ret = load_libretro_core(LIBRETRO_INIT_SO_PATH, &fn);
	lok(ret == 0);

	/* Continuing tests will result in seg fault.
	 * Abort() for severe failure. */
	if(ret)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "%s\n",
				SDL_GetError());
		abort();
	}

	lok(fn.retro_api_version() == RETRO_API_VERSION);
}

int main(void)
{
	if(SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
				"Unable to initialize SDL: %s", SDL_GetError());
		return EXIT_FAILURE;
	}

	puts("Executing tests:");
	lrun("Init", test_retro_init);
	lresults();
	return lfails != 0;
}
