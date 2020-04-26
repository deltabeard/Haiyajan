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

static Uint16 fb;
void test_retro_av_video_cb(const void *data, unsigned width, unsigned height,
			    size_t pitch)
{
	const Uint16 *buf = data;

	(void) width;
	(void) height;
	(void) pitch;

	fb = *buf;
}

void test_retro_av(void)
{
	struct timer_ctx_s tim;

	{
		/* Testing 10 FPS core with 10 Hz display. */
		timer_init(&tim, 10.0);

		lequal(timer_get_delay(&tim, 100),   0);
		lequal(timer_get_delay(&tim, 90),   10);
		lequal(timer_get_delay(&tim, 90),   10);
		lequal(timer_get_delay(&tim, 50),   50);

		lequal(timer_get_delay(&tim, 150),  -50);
		lequal(timer_get_delay(&tim, 150), -100);
		lequal(timer_get_delay(&tim, 5),     -5);
		lequal(timer_get_delay(&tim, 95),     0);
	}

	{
		/* Testing 11 FPS core with 10 Hz display. */
		timer_init(&tim, 11.0);

		lequal(timer_get_delay(&tim, 90),  0);
		lequal(timer_get_delay(&tim, 90),  1);
		lequal(timer_get_delay(&tim, 90),  0);
		lequal(timer_get_delay(&tim, 90),  1);

		lequal(timer_get_delay(&tim, 80), 10);
		lequal(timer_get_delay(&tim, 80), 11);
		lequal(timer_get_delay(&tim, 80), 10);
		lequal(timer_get_delay(&tim, 80), 11);
		lequal(timer_get_delay(&tim, 101), 0);

		timer_init(&tim, 11.0);
		for(int i = 1; i < 11; i++)
			lequal(timer_get_delay(&tim, 100), -9 * i);

		lequal(timer_get_delay(&tim, 100), -100);
	}

	{
		/* Testing 10 FPS core with 11 Hz display. */
		timer_init(&tim, 10.0);

		lequal(timer_get_delay(&tim, 100), 0);
		lequal(timer_get_delay(&tim, 99), 1);
		lequal(timer_get_delay(&tim, 101), -1);
		lequal(timer_get_delay(&tim, 5), 94);
	}

#if 0
	struct core_ctx_s ctx;
	const char av_so_path[] = "./libretro_av/libretro-av.so";
	double core_fps;
	unsigned frames;
	unsigned display;
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
		struct retro_system_av_info av_info;
		ctx.fn.retro_get_system_info(&info);
		ctx.fn.retro_get_system_av_info(&av_info);
		core_fps = av_info.timing.fps;

		lok(strcmp(info.library_name, "Test AV") == 0);
		lok(strcmp(info.library_version, "1") == 0);
		lok(info.need_fullpath == false);
		lok(info.valid_extensions == NULL);
	}

	timer_init(&tim, core_fps);

	/* Reset frame buffer. */
	fb = 0x0000;
	frames = 0;
	display = 0;
	ctx.fn.retro_init();
	ctx.fn.retro_set_video_refresh(test_retro_av_video_cb);

	do
	{
		int delay;

		if(frames % 2 == 0)
			lok(fb == 0x0000);
		else
			lok(fb == 0xFFFF);

		ctx.fn.retro_run();
		frames++;

		/* Simulate 10 Hz VSYNC. */
		delay = timer_show_frame(&tim, (1.0/10.0) * 1000.0);

		/* There should be no delay or skips, as the display has the
		 * same refresh rate as the core. */
		lequal(delay, 0);

		/* Draw frame here. */
		display++;

		/* For clarity. */
		lok(display == frames);

		/* End test after 40 frames. */
	} while(frames < 40);

	timer_init(&tim, core_fps);
	/* Reset frame buffer. */
	fb = 0x0000;
	frames = 0;
	display = 0;
	ctx.fn.retro_init();
	ctx.fn.retro_set_video_refresh(test_retro_av_video_cb);
	do
	{
		int delay = 0;

		if(frames % 2 == 0)
			lok(fb == 0x0000);
		else
			lok(fb == 0xFFFF);

		ctx.fn.retro_run();
		frames++;

		/* Simulate 8 Hz VSYNC. */
		delay = timer_show_frame(&tim, (1.0/8.0) * 1000.0);
		display++;

		/* The timer will report that the frame must be skipped. */
		lequal(delay, (int)(frames * -25));
		lok(display == frames);
	} while(frames < 10);

	timer_init(&tim, core_fps);

	/* Reset frame buffer. */
	fb = 0x0000;
	frames = 0;
	display = 0;
	ctx.fn.retro_init();
	ctx.fn.retro_set_video_refresh(test_retro_av_video_cb);

	do
	{
		int delay = 0;

		if(frames % 2 == 0)
			lok(fb == 0x0000);
		else
			lok(fb == 0xFFFF);

		ctx.fn.retro_run();
		frames++;

		/* Simulate 12 Hz VSYNC. */
		delay = timer_show_frame(&tim, (1.0/12.0) * 1000.0);

		/* There should be no delay or skips, as the display has the
		 * same refresh rate as the core. */

		/* Draw frame here. */
		display++;

		lequal(delay, (int)(frames * 17));

		/* As the display is faster than the core, we should always be
		 * able to draw the frame. */
		lok(frames == display);
	} while(frames < 10);

	timer_init(&tim, core_fps);
	fb = 0x0000;
	frames = 0;
	display = 0;
	ctx.fn.retro_init();
	ctx.fn.retro_set_video_refresh(test_retro_av_video_cb);

	/* Testing variable frame rate. */
	{
		int delay;
		/* (1/10) * 1000 = 100ms */
		delay = timer_show_frame(&tim, 125);
		lequal(delay, -25);

		delay = timer_show_frame(&tim, 50);
		lequal(delay, 25);

		delay = timer_show_frame(&tim, 125);
		lequal(delay, 0);
	}

	ctx.fn.retro_deinit();
#endif
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
