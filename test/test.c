/**
 * Test cases for the Haiyajan project.
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

#include <load.h>
#include <haiyajan.h>
#include <timer.h>

#include "minctest.h"

/**
 * Tests basic functionality:
 * - That we can load and unload a libretro core.
 * - That we can init and deinit the core.
 * - That we can receive log messages to verify the above.
 */
void test_retro_init(void)
{
	struct core_ctx_s ctx;
	const char init_so_path[] = "./libretro_init/libretro-init.so";

	/* Continuing tests will result in seg fault.
	 * Abort() for severe failure. */
	if(load_libretro_core(init_so_path, &ctx))
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "%s",
			SDL_GetError());
		abort();
	}

	{
		struct retro_system_info info;
		ctx.fn.retro_get_system_info(&info);
		lok(strcmp(info.library_name, "Init") == 0);
		lok(strcmp(info.library_version, "1") == 0);
		lok(info.need_fullpath == false);
		lok(info.valid_extensions == NULL);
		/* Not checking the other two values as they aren't specifically
		 * set. */
	}

	ctx.fn.retro_deinit();
}

static Uint16 fb = 0x0000;
void test_retro_av_video_cb(const void *data, unsigned width, unsigned height,
			    size_t pitch)
{
	static unsigned frame = 1;
	const Uint16 *buf = data;

	(void) width;
	(void) height;
	(void) pitch;

	if(frame % 2 == 0)
		fb = *buf;
	else
		fb = *buf;

	frame++;

}

void test_retro_av(void)
{
	struct core_ctx_s ctx;
	const char av_so_path[] = "./libretro_av/libretro-av.so";
	unsigned frames = 0;
	unsigned display = 0;
	struct timer_ctx_s tim;

	/* Continuing tests will result in seg fault.
	 * Abort() for severe failure. */
	if(load_libretro_core(av_so_path, &ctx))
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "%s",
			SDL_GetError());
		abort();
	}

	{
		struct retro_system_info info;
		ctx.fn.retro_get_system_info(&info);
		lok(strcmp(info.library_name, "Test AV") == 0);
		lok(strcmp(info.library_version, "1") == 0);
		lok(info.need_fullpath == false);
		lok(info.valid_extensions == NULL);
	}


	timer_init(&tim, 10.0);
	ctx.fn.retro_init();
	ctx.fn.retro_set_video_refresh(test_retro_av_video_cb);

	/* Simulating a 10 Hz display. */
	while(1)
	{
		if(frames % 2 == 0)
			lok(fb == 0x0000);
		else
			lok(fb == 0xFFFF);

		ctx.fn.retro_run();
		frames++;

		if(timer_show_frame(&tim))
		{
			/* show frame */
			display++;
		}

		if(frames == 11)
		{
			lok(display == 10);
			lok(fb == 0xFFFF);
		}
		else if(frames == 22)
		{
			lok(display == 20);
			lok(fb == 0x0000);
		}
		else if(frames == 33)
		{
			lok(display == 30);
			lok(fb == 0xFFFF);
		}

		if(frames > 40)
			break;
	}

	ctx.fn.retro_deinit();
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
	lrun("Frame Timing", test_retro_av);
	lresults();
	return lfails != 0;
}
