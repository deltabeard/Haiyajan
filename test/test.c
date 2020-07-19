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

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <font.h>
#include <haiyajan.h>
#include <load.h>
#include <menu.h>
#include <timer.h>
#include <ui.h>

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

		/* Should be perfectly in sync. */
		for(unsigned i = 10; i > 0; i--)
			lequal(timer_get_delay(&tim, 100), 0);

		lfequal(tim.core_ms, 100.0);
		lfequal(tim.timer_accumulator, 0.0);
	}

	{
		/* Testing 50.00 FPS core with 100.00 Hz display. */
		int ret = timer_init(&tim, 50.00);
		lequal(ret, 0);

		/* Our computer is really fast, so imagine that the time it
		 * takes to render a single frame is less than a milisecond. */
		lequal(timer_get_delay(&tim, 10), 0);
		lequal(timer_get_delay(&tim, 10), 0);
		lequal(timer_get_delay(&tim, 10), 0);
		lequal(timer_get_delay(&tim, 10), 0);
	}
}

void test_ui_drawing(void)
{
	SDL_Surface *ref = SDL_LoadBMP("../meta/menu_320x240.bmp");
	SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(0, ref->w, ref->h,
			ref->format->BytesPerPixel, ref->format->format);
	SDL_Renderer *rend = SDL_CreateSoftwareRenderer(surf);
	font_ctx *font = FontStartup(rend);
	ui *ui = ui_init(font, rend);
	long unsigned surfsz = surf->pitch * surf->h;
	menu_ctx menu;
	menu_item menu_items[] = {
		{ "Continue", NULL, MENU_SUB_MENU, { .sub_menu = NULL }},
		{ "Open", NULL, MENU_SUB_MENU, { .sub_menu = NULL }},
		{ "Quit", NULL, MENU_SUB_MENU, { .sub_menu = NULL }}
	};

	menu_init(&menu, NULL, NULL, NULL, SDL_arraysize(menu_items),
			menu_items);
	menu_instruct(&menu, MENU_INSTR_NEXT_ITEM);
	draw_menu(ui, &menu);

	lequal(SDL_memcmp(surf->pixels, ref->pixels, surfsz), 0);

	FontExit(font);
	SDL_DestroyRenderer(rend);
	SDL_FreeSurface(surf);
	SDL_FreeSurface(ref);
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
	lrun("UI Drawing", test_ui_drawing);
	SDL_Quit();
	lresults();
	return lfails != 0;
}
