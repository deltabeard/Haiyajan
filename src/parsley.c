/**
 * A simple and fast Libretro frontend.
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
#include <stdlib.h>

#include <load.h>
#include <play.h>

static uint_fast8_t prerun_checks(void)
{
	SDL_version compiled;
	SDL_version linked;

	SDL_VERSION(&compiled);
	SDL_GetVersion(&linked);

	if(compiled.major != linked.major)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_SYSTEM,
				"The major version of SDL2 loaded (%d) does "
				"not match the version from which Parsley was "
				"compiled with (%d). "
				"Please recompile Parsley and try again.",
				linked.major, compiled.major);
		return 1;
	}

	if(SDL_VERSIONNUM(compiled.major, compiled.minor, compiled.patch) !=
	   SDL_VERSIONNUM(linked.major, linked.minor, linked.patch))
	{
		SDL_LogWarn(
			SDL_LOG_CATEGORY_SYSTEM,
			"The version of SDL2 loaded (%d.%d.%d) is different to "
			"the version that Parsley was compiled with "
			"(%d.%d.%d).",
			linked.major, linked.minor, linked.patch,
			compiled.major, compiled.minor, compiled.patch);
	}

	return 0;
}

int main(int argc, char *argv[])
{
	struct core_ctx_s ctx = { 0 };
	char *core_path;
	char *file;
	SDL_Window *win = NULL;

	if(SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
				"SDL initialisation failed: %s",
				SDL_GetError());
		exit(EXIT_FAILURE);
	}

#ifdef DEBUG
	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
#endif

	if(prerun_checks())
		goto err;

	if(argc != 3)
	{
		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "%s CORE FILE",
			    argv[0]);
		SDL_Quit();
		exit(EXIT_FAILURE);
	}

	core_path = argv[1];
	file = argv[2];

	win = SDL_CreateWindow(PROG_NAME, SDL_WINDOWPOS_UNDEFINED,
			       SDL_WINDOWPOS_UNDEFINED, 320, 240,
			       SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

	if(win == NULL)
		goto err;

	ctx.disp_rend = SDL_CreateRenderer(
		win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	if(ctx.disp_rend == NULL)
	{
		goto err;
	}

	SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION,
		       "Created window and renderer");

	if(load_libretro_core(core_path, &ctx))
		goto err;

	/* TODO:
	 * - Check that input file is supported by core
	 */
	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
		    "Libretro core \"%.32s\" loaded successfully.",
		    ctx.sys_info.library_name);

	{
		char title[MAX_TITLE_LEN];
		SDL_snprintf(title, MAX_TITLE_LEN, "%s: %s", PROG_NAME,
			     ctx.sys_info.library_name);
		SDL_SetWindowTitle(win, title);
	}

	play_init_cb(&ctx);

	if(load_libretro_file(file, &ctx) != 0)
		goto err;

	if(play_init_av(&ctx) != 0)
		goto err;

	SDL_SetWindowMinimumSize(win, ctx.game_logical_res.w,
				 ctx.game_logical_res.h);
	SDL_RenderSetLogicalSize(ctx.disp_rend, ctx.game_logical_res.w,
				 ctx.game_logical_res.h);

	SDL_Event event;
	while(1)
	{
		SDL_PollEvent(&event);
		if(event.type == SDL_QUIT)
		{
			break;
		}

		play_frame(&ctx);

		if(ctx.game_texture != NULL)
		{
			SDL_RenderClear(ctx.disp_rend);
			SDL_RenderCopy(ctx.disp_rend, ctx.game_texture, NULL,
				       NULL);
		}

		SDL_RenderPresent(ctx.disp_rend);
	}

	unload_libretro_file(&ctx);
	unload_libretro_core(&ctx);
	play_deinit_cb(&ctx);
	SDL_DestroyRenderer(ctx.disp_rend);
	SDL_DestroyWindow(win);
	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Exiting gracefully.");
	SDL_Quit();

	exit(EXIT_SUCCESS);

err:
	SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
			"Exiting due to an error. %s", SDL_GetError());
	unload_libretro_file(&ctx);
	unload_libretro_core(&ctx);
	play_deinit_cb(&ctx);
	SDL_DestroyRenderer(ctx.disp_rend);
	SDL_DestroyWindow(win);
	SDL_Quit();
	exit(EXIT_FAILURE);
}
