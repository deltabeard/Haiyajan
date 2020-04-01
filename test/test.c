/**
 * Test cases for the Parsley project.
 * Copyright (C) 2020  Mahyar Koshkouei
 *
 * This is free software, and you are welcome to redistribute it under the terms
 * of the GNU General Public License version 3.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 *
 * See the LICENSE file for more details.
 */

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <parsley.h>
#include <load.h>

#include "minctest.h"

/**
 * Tests basic functionality:
 * - That we can load and unload a libretro core.
 * - That we can init and deinit the core.
 * - That we can receive log messages to verify the above.
 */
void test_retro_init(void)
{
	struct core_ctx_s fn;
	const char init_so_path[] = "./libretro_init/libretro-init.so";

	/* Continuing tests will result in seg fault.
	 * Abort() for severe failure. */
	if(load_libretro_core(init_so_path, &fn))
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "%s\n",
				SDL_GetError());
		abort();
	}

	fn.retro_init();
	lok(fn.retro_api_version() == load_compiled_retro_api_version());

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
