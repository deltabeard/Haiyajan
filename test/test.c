#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libretro.h>
#include <parsley.h>
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
	struct core_ctx_s fn;

	/* Continuing tests will result in seg fault.
	 * Abort() for severe failure. */
	if(load_libretro_core(LIBRETRO_INIT_SO_PATH, &fn))
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "%s\n",
				SDL_GetError());
		abort();
	}

	fn.retro_init();
	lok(fn.retro_api_version() == RETRO_API_VERSION);

	{
		struct retro_system_info info;
		fn.retro_get_system_info(&info);
		lok(strcmp(info.library_name, "Init") == 0);
		lok(strcmp(info.library_version, "1") == 0);
		lok(info.need_fullpath == false);
		lok(info.valid_extensions == NULL);
		/* Not checking the other two values as they aren't specifically
		 * set. */
	}

	fn.retro_deinit();
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
