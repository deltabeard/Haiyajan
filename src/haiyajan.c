/**
 * A simple and fast Libretro frontend.
 * Copyright (C) 2020  Mahyar Koshkouei
 *
 * This is free software, and you are welcome to redistribute it under the terms
 * of the GNU Affero General Public License version 3.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 *
 * See the LICENSE file for more details.
 */

#include <SDL2/SDL.h>
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
#include <stdio.h>
#endif

#define OPTPARSE_IMPLEMENTATION
#define OPTPARSE_API static
#include <optparse.h>

#include <font.h>
#include <input.h>
#include <load.h>
#include <play.h>
#include <timer.h>

#define PROG_NAME	"Haiyajan"
#define PROG_NAME_LEN	strlen(PROG_NAME)
#define MAX_TITLE_LEN	56

#ifndef REL_VERSION
#define REL_VERSION "UNRELEASED"
#endif
#ifndef GIT_VERSION
#define GIT_VERSION "NONE"
#endif

static void prerun_checks(void)
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

static void print_info(void)
{
	struct features_s
	{
		SDL_bool (*get_cpu_feat)(void);
		const char *const feat_name;
	};
	const struct features_s cpu_features[] =
	{
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

static void print_help(void)
{
	char str[256];
	const int num_drivers = SDL_GetNumVideoDrivers();
	const int num_rends = SDL_GetNumRenderDrivers();
	const int num_audio = SDL_GetNumAudioDrivers();

	fprintf(stderr, "\n"
		"Usage: haiyajan [OPTIONS] -L CORE FILE\n"
		"  -h, --help      Show this help message.\n"
		"      --version   Print version information.\n"
		"  -L, --libretro  Path to libretro core.\n"
		"  -I, --info      Print statistics onscreen.\n"
		"  -b, --benchmark Benchmark and print average frames per "
				"second.\n"
		"  -v, --verbose   Print verbose log messages.\n"
		"  -V, --video     Video driver to use\n"
		"\n");

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

static void free_settings(struct core_ctx_s *ctx)
{
	if(ctx->file_core != NULL)
	{
		SDL_free(ctx->file_core);
		ctx->file_core = NULL;
	}

	if(ctx->file_content != NULL)
	{
		SDL_free(ctx->file_content);
		ctx->file_content = NULL;
	}
}

static void apply_settings(char **argv, struct core_ctx_s *ctx)
{
	const struct optparse_long longopts[] =
	{
		{ "libretro",	'L', OPTPARSE_REQUIRED },
		{ "info",	'I', OPTPARSE_NONE },
		{ "verbose",	'v', OPTPARSE_NONE },
		{ "video",	'V', OPTPARSE_REQUIRED },
		{ "version",	 1 , OPTPARSE_NONE },
		{ "benchmark",	'b', OPTPARSE_OPTIONAL },
		{ "help",	'h', OPTPARSE_NONE },
		{ 0 }
	};
	int option;
	struct optparse options;
	char *rem_arg;
	uint_fast8_t video_init = 0;

	optparse_init(&options, argv);

	while((option = optparse_long(&options, longopts, NULL)) != -1)
	{
		switch(option)
		{
		case 'L':
			ctx->file_core = SDL_strdup(options.optarg);
			break;

		case 'I':
			ctx->stngs.vid_info = 1;
			break;

		case 'v':
			SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
			break;

		case 'V':
			if(video_init)
			{
				SDL_LogInfo(SDL_LOG_CATEGORY_VIDEO,
					"Previously initialised video driver "
					"%s will be replaced with %s",
					SDL_GetCurrentVideoDriver(),
					    options.optarg);
				SDL_VideoQuit();
				video_init = 0;
			}

			if(SDL_VideoInit(options.optarg) != 0)
			{
				SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO,
					"Unable to initialise specified "
					"video driver: %s", SDL_GetError());
			}
			else
			{
				SDL_LogInfo(SDL_LOG_CATEGORY_VIDEO,
					"%s was successfully initialised",
					options.optarg);
				video_init = 1;
			}

			break;

		case 1:
			/* Version information has already been printed. */
			exit(EXIT_SUCCESS);

		case 'b':
			ctx->stngs.benchmark = 1;
			if(options.optarg != 0)
				ctx->stngs.benchmark_dur = SDL_atoi(options.optarg);

			if(ctx->stngs.benchmark_dur == 0)
				ctx->stngs.benchmark_dur = 20;

			SDL_LogInfo(SDL_LOG_CATEGORY_VIDEO,
					"Haiyajan will exit after performing a "
					"benchmark for %d seconds",
					ctx->stngs.benchmark_dur);
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
	rem_arg = optparse_arg(&options);

	if(rem_arg != NULL)
		ctx->file_content = SDL_strdup(rem_arg);
	else
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
			"The path to the content file was not given");
		goto err;
	}

	if(ctx->file_core == NULL)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
			"The path to a libretro core was not given");
		goto err;
	}

	/* Initialise default video driver if not done so already. */
	if(video_init == 0 && SDL_VideoInit(NULL) != 0)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
			"Unable to initialise a video driver: %s",
			SDL_GetError());
		goto err;
	}

	return;

err:
	print_help();
	exit(EXIT_FAILURE);
}

static SDL_atomic_t screencap_timeout;

Uint32 enable_screencapture(Uint32 interval, void *param)
{
	SDL_AtomicSet(&screencap_timeout, 0);
	(void)param;
	return interval;
}

void save_texture(SDL_Renderer *rend, SDL_Texture *tex,
		  const SDL_Rect *const src,
		  const char *const filename)
{
	SDL_Texture *core_tex;
	SDL_Surface *surf = NULL;
	int format = SDL_PIXELFORMAT_RGB888;

	core_tex = SDL_CreateTexture(rend, format, SDL_TEXTUREACCESS_TARGET,
				    src->w, src->h);
	if(core_tex == NULL)
		goto err;

	if(SDL_SetRenderTarget(rend, core_tex) != 0)
		goto err;

	if(SDL_RenderCopy(rend, tex, src, src) != 0)
		goto err;

	surf = SDL_CreateRGBSurfaceWithFormat(0, src->w, src->h,
					      SDL_BITSPERPIXEL(format), format);
	if(!surf)
		goto err;

	if(SDL_RenderReadPixels(rend, src, format, surf->pixels, surf->pitch) != 0)
		goto err;

	if(SDL_SaveBMP(surf, filename) != 0)
		goto err;

	SDL_Log("Saved texture as BMP to \"%s\"\n", filename);

err:
	SDL_FreeSurface(surf);
	SDL_DestroyTexture(core_tex);
}

void take_screencapture(struct core_ctx_s *const ctx)
{
	const Uint32 interval_ms = 1000;
	time_t now;
	struct tm *tmp;
	char time_str[32];
	char filename[64];

	/* Screencaptures limited to 1 every second. Should not be used for
	 * recording game play as a video, as this function is too slow for
	 * that. */
	if(SDL_AtomicGet(&screencap_timeout))
		return;

	SDL_AtomicSet(&screencap_timeout, 1);
	SDL_AddTimer(interval_ms, enable_screencapture, NULL);

	now = time(NULL);
	if(now == (time_t) -1 || (tmp = localtime(&now)) == NULL ||
		strftime(time_str, sizeof(time_str), "%Y-%m-%d-%H%M%S", tmp) == 0)
	{
		SDL_snprintf(time_str, sizeof(time_str), "%010u",
					 SDL_GetTicks());
	}

	SDL_snprintf(filename, sizeof(filename), "%s-%s.bmp",
				 time_str, ctx->core_log_name);

	save_texture(ctx->disp_rend, ctx->core_tex, &ctx->game_frame_res, filename);

	return;
}

static void run(struct core_ctx_s *ctx)
{
	font_ctx *font;
	SDL_Event ev;
	Uint32 ticks_before, ticks_next, delta_ticks;
	struct timer_ctx_s tim;
	int tim_cmd;
	float fps = 0.0F;
	Uint32 fps_beg;
	const uint_fast8_t fps_calc_frame_dur = 64;
	uint_fast8_t fps_curr_frame_dur = fps_calc_frame_dur;
	Uint32 benchmark_beg;

	input_init(&ctx->inp);
	ctx->fn.retro_set_controller_port_device(0, RETRO_DEVICE_JOYPAD);

	font = FontStartup(ctx->disp_rend);
	if(font == NULL)
	{
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
			"Unable to start font drawer: %s", SDL_GetError());
		/* Disable font drawing on error. */
		ctx->stngs.vid_info = 0;
	}

	tim_cmd = timer_init(&tim, ctx->av_info.timing.fps);
	ticks_before = fps_beg = benchmark_beg = SDL_GetTicks();

	while(ctx->env.status_bits.shutdown == 0)
	{
		static Uint32 frames = 0;

		while(SDL_PollEvent(&ev) != 0)
		{
			if(ev.type == SDL_QUIT)
				goto out;
			else if(INPUT_EVENT_CHK(ev.type))
				input_handle_event(&ctx->inp, &ev);
			else if(ev.type == ctx->inp.input_cmd_event &&
				!ctx->stngs.benchmark)
			{
				switch(ev.user.code)
				{
				case INPUT_EVENT_TOGGLE_INFO:
					ctx->stngs.vid_info = !ctx->stngs.vid_info;
					SDL_SetRenderDrawBlendMode(ctx->disp_rend,
								   ctx->stngs.vid_info ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE);
					if(!ctx->stngs.vid_info)
						break;

					/* Reset FPS counter. */
					fps_beg = SDL_GetTicks();
					fps_curr_frame_dur = fps_calc_frame_dur;
					break;

				case INPUT_EVENT_TOGGLE_FULLSCREEN:
					ctx->stngs.fullscreen = !ctx->stngs.fullscreen;
					if(ctx->stngs.fullscreen)
						SDL_SetWindowFullscreen(ctx->win, SDL_WINDOW_FULLSCREEN_DESKTOP);
					else
						SDL_SetWindowFullscreen(ctx->win, 0);
					break;

				case INPUT_EVENT_TAKE_SCREENCAPTURE:
					take_screencapture(ctx);
					break;
				}
			}
		}

		/* If in benchmark mode, run as fast as possible. */
		if(ctx->stngs.benchmark)
			tim_cmd = 0;

		frames++;

		if(tim_cmd < 0)
		{
			/* Disable video for the skipped frame to improve
			 * performance. */
			ctx->env.status_bits.video_disabled = 1;
			play_frame(ctx);
			ctx->env.status_bits.video_disabled = 0;

			/* Skip VSYNC. */
			goto timing;
		}

		if(tim_cmd > 0)
			SDL_Delay(tim_cmd);

		SDL_RenderClear(ctx->disp_rend);
		play_frame(ctx);
		SDL_RenderCopyEx(ctx->disp_rend, ctx->core_tex,
				 &ctx->game_frame_res, NULL, 0.0, NULL,
				 ctx->env.flip);

		if(ctx->stngs.vid_info)
		{
			static char busy_str[10] = { '\0' };
			static char fps_str[10] = { '\0' };
			static char acc_str[10] = { '\0' };
			static char aud_str[10] = { '\0' };
			static char frames_str[10] = { '\0' };
			const SDL_Rect font_bg =
			{
				.w = 10 * FONT_CHAR_WIDTH,
				.h = 5 * (FONT_CHAR_HEIGHT + 1),
				.x = 0,
				.y = 0
			};
			SDL_Rect dim = { .w = 1, .h = 1, .x = 0, .y = 0 };
			Uint32 ticks_busy = SDL_GetTicks();
			Uint32 busy_diff = ticks_busy - ticks_before;

			/* Only update every five frames to make values
			 * readable. */
			if(fps_curr_frame_dur % 5 == 0)
			{
				SDL_snprintf(busy_str, 10, "%6u ms", busy_diff);
				SDL_snprintf(acc_str, 10, "%6.2f ms",
					tim.timer_accumulator);
				SDL_snprintf(aud_str, 10, "%6" PRIu32,
					(Uint32)(SDL_GetQueuedAudioSize(ctx->audio_dev) /
					sizeof(Uint16) / 2U));
				SDL_snprintf(frames_str, 10, "%6u", frames);
			}

			/* Update only after FPS has been recalculated. */
			if(fps_curr_frame_dur == fps_calc_frame_dur)
				SDL_snprintf(fps_str, 10, "%6.2f Hz", fps);

			SDL_SetRenderDrawColor(ctx->disp_rend,
				0x00, 0x00, 0x00, 0x40);
			SDL_RenderFillRect(ctx->disp_rend, &font_bg);

			SDL_SetRenderDrawColor(ctx->disp_rend,
				0xFF, 0xFF, 0xFF, 0xFF);
			FontPrintToRenderer(font, busy_str, &dim);

			dim.y += FONT_CHAR_HEIGHT + 1;
			FontPrintToRenderer(font, fps_str, &dim);

			dim.y += FONT_CHAR_HEIGHT + 1;
			FontPrintToRenderer(font, acc_str, &dim);

			dim.y += FONT_CHAR_HEIGHT + 1;
			FontPrintToRenderer(font, aud_str, &dim);

			dim.y += FONT_CHAR_HEIGHT + 1;
			FontPrintToRenderer(font, frames_str, &dim);

			SDL_SetRenderDrawColor(ctx->disp_rend,
				0x00, 0x00, 0x00, 0x00);
		}

		/* Only draw to screen if we're not falling behind. */
		SDL_RenderPresent(ctx->disp_rend);

timing:
		ticks_next = SDL_GetTicks();
		delta_ticks = ticks_next - ticks_before;
		tim_cmd = timer_get_delay(&tim, delta_ticks);
		ticks_before = ticks_next;

		if(ctx->stngs.vid_info)
			fps_curr_frame_dur--;

		if(ctx->stngs.vid_info && fps_curr_frame_dur == 0)
		{
			Uint32 fps_delta;
			Uint32 fps_end = SDL_GetTicks();
			fps_delta = fps_end - fps_beg;
			fps = 1000.0F / ((float)fps_delta / (float)fps_calc_frame_dur);
			fps_curr_frame_dur = fps_calc_frame_dur;
			fps_beg = fps_end;
		}

		while(ctx->stngs.benchmark)
		{
			uint_fast32_t elapsed;
			float bench_fps;
			static Uint32 bench_frames = 0;

			bench_frames++;
			elapsed = SDL_GetTicks() - benchmark_beg;
			if(elapsed < ctx->stngs.benchmark_dur * 1000)
				break;

			bench_fps = (float)bench_frames /
					((float)elapsed / 1000.0F);
			SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
					"Benchmark: %.2f FPS", bench_fps);
			goto out;
		}
	}

out:
	FontExit(font);
}

int main(int argc, char *argv[])
{
	int ret = EXIT_FAILURE;
	struct core_ctx_s ctx = { 0 };

	/* Ignore argc being unused warning. */
	(void)argc;

#ifdef _WIN32
	/* Windows (MinGW) does not unbuffer stderr by default. */
	setbuf(stderr, NULL);
#endif

	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_INFO);

	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
		"%s Libretro Interface -- %s (GIT %s)\n", PROG_NAME,
		REL_VERSION, GIT_VERSION);

	print_info();
	prerun_checks();

#if SDL_VERSION_ATLEAST(2, 0, 13)
	SDL_SetHint(SDL_HINT_AUDIO_DEVICE_APP_NAME, PROG_NAME);
#endif

	if(SDL_Init(SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER | SDL_INIT_TIMER) != 0)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
			"SDL initialisation failed: %s",
			SDL_GetError());
		exit(EXIT_FAILURE);
	}

	apply_settings(argv, &ctx);

	ctx.win = SDL_CreateWindow(PROG_NAME, SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED, 320, 240,
			SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

	if(ctx.win == NULL)
		goto err;

	{
		Uint32 flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE;
		if(!ctx.stngs.benchmark)
			flags |= SDL_RENDERER_PRESENTVSYNC;

		ctx.disp_rend = SDL_CreateRenderer(ctx.win, -1, flags);
	}

	if(ctx.disp_rend == NULL)
		goto err;

#if 0
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES , 4);
#endif

	/* Allows for transparency on information display. */
	if(ctx.stngs.vid_info)
		SDL_SetRenderDrawBlendMode(ctx.disp_rend, SDL_BLENDMODE_BLEND);

	SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION,
		"Created window and renderer");

	if(load_libretro_core(ctx.file_core, &ctx))
		goto err;

	/* TODO:
	 * - Check that input file is supported by core
	 */
	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
		"Libretro core \"%.32s\" loaded successfully.",
		ctx.sys_info.library_name);

#if SDL_VERSION_ATLEAST(2, 0, 13)
	SDL_SetHint(SDL_HINT_AUDIO_DEVICE_STREAM_NAME, ctx.sys_info.library_name);
#endif

	{
		char title[MAX_TITLE_LEN];
		SDL_snprintf(title, MAX_TITLE_LEN, "%s: %s", PROG_NAME,
			ctx.sys_info.library_name);
		SDL_SetWindowTitle(ctx.win, title);
	}

	play_init_cb(&ctx);

	if(load_libretro_file(&ctx) != 0)
		goto err;

	if(play_init_av(&ctx) != 0)
		goto err;

	SDL_SetWindowMinimumSize(ctx.win, ctx.game_max_res.w,
		ctx.game_max_res.h);
	SDL_SetWindowSize(ctx.win, ctx.game_max_res.w, ctx.game_max_res.h);
	SDL_RenderSetLogicalSize(ctx.disp_rend, ctx.game_max_res.w,
		ctx.game_max_res.h);
	run(&ctx);

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
	SDL_DestroyWindow(ctx.win);
	SDL_VideoQuit();
	SDL_Quit();
	free_settings(&ctx);

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
