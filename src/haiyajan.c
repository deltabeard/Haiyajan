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

#ifdef _WIN32
#include <stdio.h>
#endif

#define OPTPARSE_IMPLEMENTATION
#define OPTPARSE_API static
#include <optparse.h>

#include <load.h>
#include <play.h>

struct cmd_args_s
{
	char *file_core;
	char *file_content;
	unsigned char benchmark : 1;
};

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
				"not match the version from which Haiyajan was "
				"compiled with (%d). "
				"Please recompile Haiyajan and try again.",
				linked.major, compiled.major);
		return 1;
	}

	if(SDL_VERSIONNUM(compiled.major, compiled.minor, compiled.patch) !=
	   SDL_VERSIONNUM(linked.major, linked.minor, linked.patch))
	{
		SDL_LogWarn(
			SDL_LOG_CATEGORY_SYSTEM,
			"The version of SDL2 loaded (%d.%d.%d) is different to "
			"the version that Haiyajan was compiled with "
			"(%d.%d.%d).",
			linked.major, linked.minor, linked.patch,
			compiled.major, compiled.minor, compiled.patch);
	}

	return 0;
}

static void print_info(void)
{
	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
		    PROG_NAME " Libretro Interface -- " REL_VERSION
		    " (" GIT_VERSION ")\n");
}

static void print_help(const char *app_name)
{
	char buf[256] = "\0";
	const int num_drivers = SDL_GetNumVideoDrivers();

	for(int index = 0; index < num_drivers; index++)
	{
		if(index != 0)
			SDL_strlcat(buf, ", ", SDL_arraysize(buf));

		SDL_strlcat(buf, SDL_GetVideoDriver(index), SDL_arraysize(buf));
	}

	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
		    "Usage: %s [OPTIONS] -L CORE FILE\n"
		    "  -h, --help      Show this help message.\n"
		    "  -b, --benchmark Measures how many frames are rendered "
		    "within 5 seconds.\n"
		    "  -L, --libretro  Path to libretro core.\n"
	            "\n"
		    "\n"
	     "Available video drivers: %s\n",
	     app_name, buf);
}

static uint_fast8_t process_args(int argc, char **argv, struct cmd_args_s *args)
{
	const struct optparse_long longopts[] = {
		{ "libretro", 'L', OPTPARSE_REQUIRED },
		{ "benchmark", 'b', OPTPARSE_NONE },
		{ "help", 'h', OPTPARSE_NONE },
		{ 0 }
	};
	int option;
	struct optparse options;

	SDL_memset(args, 0, sizeof(struct cmd_args_s));

	optparse_init(&options, argv);
	while((option = optparse_long(&options, longopts, NULL)) != -1)
	{
		switch(option)
		{
		case 'L':
			args->file_core = SDL_strdup(options.optarg);
			break;

		case 'b':
			args->benchmark = 1;
			break;

		case 'h':
			print_info();
			print_help(argv[0]);
			exit(EXIT_SUCCESS);

		case '?':
			SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "%s",
					options.errmsg);
			return 1;
		}
	}

	/* Print remaining arguments. */
	char *arg = optparse_arg(&options);
	if(arg != NULL)
		args->file_content = SDL_strdup(arg);

	return 0;
}

int main(int argc, char *argv[])
{
	struct core_ctx_s ctx = { 0 };
	SDL_Window *win = NULL;
	struct cmd_args_s args;

#ifdef _WIN32
	/* Windows (MinGW) does not unbuffer stderr by default. */
	setbuf(stderr, NULL);
#endif

	if(process_args(argc, argv, &args) != 0 ||
		args.file_core == NULL || args.file_content == NULL)
	{
		if(args.file_core == NULL)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
					"The path to a libretro core was not given");
		}

		if(args.file_content == NULL)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
					"The path to the content file was not given");
		}

		print_info();
		print_help(argv[0]);
		exit(EXIT_FAILURE);
	}

	print_info();

	if(prerun_checks() != 0)
		exit(EXIT_FAILURE);

	if(SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
				"SDL initialisation failed: %s",
				SDL_GetError());
		exit(EXIT_FAILURE);
	}

#ifdef DEBUG
	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
#else
	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_INFO);
#endif

	win = SDL_CreateWindow(PROG_NAME, SDL_WINDOWPOS_UNDEFINED,
			       SDL_WINDOWPOS_UNDEFINED, 320, 240,
			       SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

	if(win == NULL)
		goto err;

	ctx.disp_rend = SDL_CreateRenderer(
		win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	if(ctx.disp_rend == NULL)
		goto err;

	SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION,
		       "Created window and renderer");

	if(load_libretro_core(args.file_core, &ctx))
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

	if(load_libretro_file(args.file_content, &ctx) != 0)
	{
		unload_libretro_core(&ctx);
		goto err;
	}

	if(play_init_av(&ctx) != 0)
		goto err;

	SDL_SetWindowMinimumSize(win, ctx.game_logical_res.w,
				 ctx.game_logical_res.h);
	SDL_SetWindowSize(win, ctx.game_logical_res.w, ctx.game_logical_res.h);
	SDL_RenderSetLogicalSize(ctx.disp_rend, ctx.game_logical_res.w,
				 ctx.game_logical_res.h);
	// SDL_RenderSetIntegerScale(ctx.disp_rend, SDL_ENABLE);

	SDL_Event event;
	while(1)
	{
		SDL_PollEvent(&event);
		if(event.type == SDL_QUIT)
		{
			break;
		}

		play_frame(&ctx);

		if(ctx.core_tex != NULL)
		{
			SDL_RenderClear(ctx.disp_rend);
			SDL_RenderCopy(ctx.disp_rend, ctx.core_tex, NULL, NULL);
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
