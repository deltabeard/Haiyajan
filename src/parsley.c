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
	struct core_ctx_s ctx;
	char *file;
	SDL_Window *win;
	SDL_Renderer *rend;

	if(SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
				"SDL initialisation failed: %s",
				SDL_GetError());
		exit(EXIT_FAILURE);
	}

#ifdef DEBUG
	SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION,
			   SDL_LOG_PRIORITY_VERBOSE);
#endif

	if(prerun_checks())
		goto err;

	if(argc != 3)
	{
		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "%s CORE FILE",
			    argv[0]);
		goto err;
	}

	file = argv[2];

	if(load_libretro_core(argv[1], &ctx))
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "%s",
				SDL_GetError());
		goto err;
	}

	if(load_libretro_file(file, &ctx) != 0)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "%s",
				SDL_GetError());
		goto err;
	}

	/* TODO:
	 * - Check that input file is supported by core
	 */
	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
		    "Libretro core \"%.32s\" loaded successfully.",
		    ctx.sys_info.library_name);

	SDL_CreateWindowAndRenderer(ctx.av_info.geometry.base_width,
				    ctx.av_info.geometry.base_height, 0, &win,
				    &rend);
	SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION,
		       "Created window and renderer %u*%u",
		       ctx.av_info.geometry.base_width,
		       ctx.av_info.geometry.base_height);

	{
		char title[56];
		snprintf(title, 56, "Parsley: %.32s", ctx.sys_info.library_name);
		SDL_SetWindowTitle(win, title);
	}

	ctx.game_texture = SDL_CreateTexture(rend, ctx.env.pixel_fmt,
					     SDL_TEXTUREACCESS_STREAMING,
				      ctx.av_info.geometry.base_width,
				      ctx.av_info.geometry.base_height);
	if(ctx.game_texture == NULL)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
			"Unable to create texture");
		goto err;
	}

	SDL_Event event;
	while(1)
	{
		SDL_PollEvent(&event);
		if(event.type == SDL_QUIT)
		{
			break;
		}

		SDL_RenderClear(rend);
		play_frame();
#if 0
		SDL_Texture *tex =
			SDL_CreateTextureFromSurface(rend, ctx.game_surface);
#endif
		SDL_RenderCopy(rend, ctx.game_texture, NULL, NULL);
		SDL_RenderPresent(rend);
		SDL_Delay(15);
	}

	unload_libretro_file(&ctx);
	unload_libretro_core(&ctx);
	SDL_Quit();
	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Exiting gracefully.");

	exit(EXIT_SUCCESS);

err:
	SDL_Quit();
	exit(EXIT_FAILURE);
}
