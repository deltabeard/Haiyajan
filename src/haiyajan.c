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

#ifdef __GNUC__
#define FUNC_OPTIMIZE_SMALL	__attribute__((optimize("Os")))
#else
#define FUNC_OPTIMIZE_SMALL
#endif

#define BENCHMARK_DUR_SEC	20

struct cmd_args_s
{
	char *file_core;
	char *file_content;
	unsigned char benchmark : 1;
};

static void FUNC_OPTIMIZE_SMALL prerun_checks(void)
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
		exit(EXIT_FAILURE);
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
}

static void FUNC_OPTIMIZE_SMALL print_info(void)
{
	struct features_s {
		SDL_bool (*get_cpu_feat)(void);
		const char *const feat_name;
	};
	const struct features_s cpu_features[] = {
		{ SDL_Has3DNow,	"3DNow"	},
		{ SDL_HasAVX,	"AVX"	},
		{ SDL_HasAVX2,	"AVX2"	},
		{ SDL_HasAltiVec, "VMX"	},
		{ SDL_HasMMX,	"MMX"	},
		{ SDL_HasRDTSC,	"RDTSC"	},
		{ SDL_HasSSE,	"SSE"	},
		{ SDL_HasSSE2,	"SSE2"	},
		{ SDL_HasSSE3,	"SSE3"	},
		{ SDL_HasSSE41,	"SSE41"	},
		{ SDL_HasSSE42,	"SSE42"	}
	};
	char str_feat[128] = "\0";

	for(size_t i = 0; i < SDL_arraysize(cpu_features); i++)
	{
		if(cpu_features[i].get_cpu_feat() == SDL_FALSE)
			continue;

		SDL_strlcat(str_feat, cpu_features[i].feat_name,
			    SDL_arraysize(str_feat));
		SDL_strlcat(str_feat, " ", SDL_arraysize(str_feat));
	}

	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
	"%s platform, %d core CPU, featuring %s\n",
		    SDL_GetPlatform(), SDL_GetCPUCount(),
		    str_feat[0] == '\0' ? "no additional instructions" : str_feat);
}

static void FUNC_OPTIMIZE_SMALL print_help(void)
{
	char str[256];
	const int num_drivers = SDL_GetNumVideoDrivers();
	const int num_rends = SDL_GetNumRenderDrivers();
	const int num_audio = SDL_GetNumAudioDrivers();

	fprintf(stderr, "\n"
			"Usage: haiyajan [OPTIONS] -L CORE FILE\n"
			"  -h, --help      Show this help message.\n"
			"  -v, --verbose   Print verbose log messages.\n"
			"  -b, --benchmark Measures how many frames are "
			"rendered within %d seconds.\n"
			"  -L, --libretro  Path to libretro core.\n"
			"\n", BENCHMARK_DUR_SEC);

	str[0] = '\0';
	for(int index = 0; index < num_drivers; index++)
	{
		if(index != 0)
			SDL_strlcat(str, ", ", SDL_arraysize(str));

		SDL_strlcat(str, SDL_GetVideoDriver(index),
			    SDL_arraysize(str));
	}
	fprintf(stderr, "Available video drivers: %s\n",
			num_drivers >= 0 ? str : "Error");

	str[0] = '\0';
	for(int index = 0; index < num_rends; index++)
	{
		SDL_RendererInfo info;

		if(index != 0)
			SDL_strlcat(str, ", ", SDL_arraysize(str));

		SDL_GetRenderDriverInfo(index, &info);
		SDL_strlcat(str, info.name, SDL_arraysize(str));
	}
	fprintf(stderr, "Available render drivers: %s\n",
			num_rends >= 0 ? str : "Error");

	str[0] = '\0';
	for(int index = 0; index < num_audio; index++)
	{
		if(index != 0)
			SDL_strlcat(str, ", ", SDL_arraysize(str));

		SDL_strlcat(str, SDL_GetAudioDriver(index), SDL_arraysize(str));
	}
	fprintf(stderr, "Available audio output devices: %s\n",
			num_audio >= 0 ? str : "Error");

	fprintf(stderr, "The following environment variables may be used to "
			"select a specific driver:\n"
			"  SDL_VIDEODRIVER\n"
			"  SDL_RENDER_DRIVER\n"
			"  SDL_AUDIODRIVER\n");
}

static void FUNC_OPTIMIZE_SMALL process_args(char **argv, struct cmd_args_s *args)
{
	const struct optparse_long longopts[] = {
		{ "libretro", 'L', OPTPARSE_REQUIRED },
		{ "verbose", 'v', OPTPARSE_NONE },
		{ "benchmark", 'b', OPTPARSE_NONE },
		{ "help", 'h', OPTPARSE_NONE },
		{ 0 }
	};
	int option;
	struct optparse options;

	optparse_init(&options, argv);
	while((option = optparse_long(&options, longopts, NULL)) != -1)
	{
		switch(option)
		{
		case 'L':
			args->file_core = SDL_strdup(options.optarg);
			break;

		case 'v':
			SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
			break;

		case 'b':
			args->benchmark = 1;
			break;

		case 'h':
			print_help();
			exit(EXIT_SUCCESS);

		case '?':
			SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "%s",
					options.errmsg);
			goto err;
		}
	}

	/* Print remaining arguments. */
	char *arg = optparse_arg(&options);
	if(arg != NULL)
		args->file_content = SDL_strdup(arg);
	else
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
				"The path to the content file was not given");
		goto err;
	}

	if(args->file_core == NULL)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
				"The path to a libretro core was not given");
		goto err;
	}

	return;

err:
	print_help();
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	int ret = EXIT_FAILURE;
	struct core_ctx_s ctx = { 0 };
	struct cmd_args_s args = { 0 };
	SDL_Window *win = NULL;

	/* Ignore argc being unused warning. */
	(void) argc;

#ifdef _WIN32
	/* Windows (MinGW) does not unbuffer stderr by default. */
	setbuf(stderr, NULL);
#endif

#ifdef DEBUG
	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
#else
	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_INFO);
#endif

	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "%s\n",
			PROG_NAME " Libretro Interface -- " REL_VERSION
			" (" GIT_VERSION ")");

	process_args(argv, &args);
	print_info();
	prerun_checks();

	if(SDL_Init(SDL_INIT_AUDIO | SDL_INIT_EVENTS) != 0)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
				"SDL initialisation failed: %s",
				SDL_GetError());
		exit(EXIT_FAILURE);
	}

	if(SDL_VideoInit(args.benchmark ? "offscreen" : NULL) != 0)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
				"Error initializing SDL video:  %s\n",
				SDL_GetError());
		exit(EXIT_FAILURE);
	}

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
		goto err;

	if(play_init_av(&ctx) != 0)
		goto err;

	SDL_SetWindowMinimumSize(win, ctx.game_logical_res.w,
				 ctx.game_logical_res.h);
	SDL_SetWindowSize(win, ctx.game_logical_res.w, ctx.game_logical_res.h);
	SDL_RenderSetLogicalSize(ctx.disp_rend, ctx.game_logical_res.w,
				 ctx.game_logical_res.h);
	// SDL_RenderSetIntegerScale(ctx.disp_rend, SDL_ENABLE);

	if(args.benchmark)
	{
		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
			    "Running benchmark for %d seconds, please wait.",
			    BENCHMARK_DUR_SEC);
	}

	SDL_Event event;
	const Uint32 start_ticks = SDL_GetTicks();
	Uint32 delta_ticks = 1;
	uint_fast32_t frames = 0;

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

		if(args.benchmark == 0)
			continue;

		/* Whilst running benchmark, flush the audio queue when it
		 * increases above 128 KiB to reduce memory usage. */
		if(SDL_GetQueuedAudioSize(ctx.audio_dev) > (128 * 1024))
			SDL_ClearQueuedAudio(ctx.audio_dev);

		frames++;
		if((delta_ticks = (SDL_GetTicks() - start_ticks)) >= (20 * 1000))
			break;
	}

	if(args.benchmark)
	{
		double fps = (double)frames / ((double)delta_ticks / 1000.0);
		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "FPS: %.2f", fps);
	}

	ret = EXIT_SUCCESS;

out:
	if(ctx.env.status_bits.game_loaded)
		unload_libretro_file(&ctx);

	if(ctx.env.status_bits.core_init)
	{
		unload_libretro_core(&ctx);
		play_deinit_cb(&ctx);
	}

	SDL_DestroyRenderer(ctx.disp_rend);
	SDL_DestroyWindow(win);
	SDL_Quit();

	if(ret == EXIT_SUCCESS)
	{
		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
			    "Exiting gracefully.");
	}

	exit(ret);

err:
	SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
			"Exiting due to an error. %s", SDL_GetError());

	goto out;
}
