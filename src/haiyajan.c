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

#include <SDL.h>
#include <stdlib.h>

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
#include <rec.h>
#include <timer.h>
#include <util.h>
#include <sig.h>
#include <ui.h>

#define PROG_NAME       "Haiyajan"
#define MAX_TITLE_LEN   56

#ifndef GIT_VERSION
#define GIT_VERSION     "NONE"
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
	struct features_s {
		SDL_bool (*get_cpu_feat)(void);
		const char *const feat_name;
	};
	const struct features_s cpu_features[] = {
		{SDL_Has3DNow,   "3DNow"},
		{SDL_HasAVX,     "AVX"},
		{SDL_HasAVX2,    "AVX2"},
		{SDL_HasAltiVec, "VMX"},
		{SDL_HasMMX,     "MMX"},
		{SDL_HasRDTSC,   "RDTSC"},
		{SDL_HasSSE,     "SSE"},
		{SDL_HasSSE2,    "SSE2"},
		{SDL_HasSSE3,    "SSE3"},
		{SDL_HasSSE41,   "SSE41"},
		{SDL_HasSSE42,   "SSE42"}
	};
	char str_feat[128] = "\0";
	unsigned i;

	for(i = 0; i < SDL_arraysize(cpu_features); i++)
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
		    str_feat[0] == '\0' ? "no additional instructions"
					: str_feat);
}

static void print_help(void)
{
	const int num_drivers = SDL_GetNumVideoDrivers();
	const int num_rends = SDL_GetNumRenderDrivers();
	const int num_audio = SDL_GetNumAudioDrivers();
	int i;

	fprintf(stderr, "\n"
			"Usage: haiyajan [OPTIONS] -L CORE FILE\n"
			"  -h, --help      Show this help message.\n"
			"      --version   Print version information.\n"
			"  -L, --libretro  Path to libretro core.\n"
			"  -I, --info      Print statistics onscreen.\n"
			"  -b, --benchmark Benchmark and print average frames"
			" per second.\n"
			"  -v, --verbose   Print verbose log messages.\n"
			"  -V, --video     Video driver to use\n"
			"  -R, --render    Render driver to use\n");

	for(i = 0; i < num_drivers; i++)
	{
		fprintf(stderr, "%s%s", i != 0 ?
					", " : "\nAvailable video drivers: ",
			SDL_GetVideoDriver(i));
	}

	for(i = 0; i < num_rends; i++)
	{
		SDL_RendererInfo info;
		SDL_GetRenderDriverInfo(i, &info);
		fprintf(stderr, "%s%s", i != 0 ?
					", " : "\nAvailable render drivers: ",
			info.name);
	}

	for(i = 0; i < num_audio; i++)
	{
		fprintf(stderr, "%s%s", i != 0 ?
					", " : "\nAvailable audio drivers: ",
			SDL_GetAudioDriver(i));
	}

	fprintf(stderr, "\nThe following environment variables may be used to "
			"select a specific driver:\n"
			"  SDL_VIDEODRIVER\n"
			"  SDL_RENDER_DRIVER\n"
			"  SDL_AUDIODRIVER\n");
}

static void free_settings(struct core_ctx_s *ctx)
{
	if(ctx->core_filename != NULL)
	{
		SDL_free(ctx->core_filename);
		ctx->core_filename = NULL;
	}

	if(ctx->content_filename != NULL)
	{
		SDL_free(ctx->content_filename);
		ctx->content_filename = NULL;
	}
}

static void apply_settings(char **argv, struct settings_s *cfg)
{
	const struct optparse_long longopts[] = {
			{"libretro",  'L', OPTPARSE_REQUIRED},
			{"info",      'I', OPTPARSE_NONE},
			{"verbose",   'v', OPTPARSE_NONE},
			{"video",     'V', OPTPARSE_REQUIRED},
			{"render",    'R', OPTPARSE_REQUIRED},
			{"version",   1,   OPTPARSE_NONE},
			{"benchmark", 'b', OPTPARSE_OPTIONAL},
			{"help",      'h', OPTPARSE_NONE},
			{0}
		};
	int option;
	struct optparse options;
	char *rem_arg;
	Uint8 video_init = 0;

	optparse_init(&options, argv);

	while((option = optparse_long(&options, longopts, NULL)) != -1)
	{
		switch(option)
		{
		case 'L':
			cfg->core_filename = SDL_strdup(options.optarg);
			break;

		case 'I':
			cfg->vid_info = 1;
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

		case 'R':
			SDL_SetHint(SDL_HINT_RENDER_DRIVER, options.optarg);
			break;

		case 1:
			/* Version information has already been printed. */
			exit(EXIT_SUCCESS);

		case 'b':
			cfg->benchmark = 1;
			if(options.optarg != 0)
				cfg->benchmark_dur =
					SDL_atoi(options.optarg);

			if(cfg->benchmark_dur == 0)
				cfg->benchmark_dur = 20;

			SDL_LogInfo(SDL_LOG_CATEGORY_VIDEO,
				    "Haiyajan will exit after performing a "
				    "benchmark for %d seconds",
				    cfg->benchmark_dur);
			break;

		case 'h':
			print_help();
			exit(EXIT_SUCCESS);

		case '?':
			SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "%s",
					options.errmsg);
			goto err;

		default:
			break;
		}
	}

	/* Print remaining arguments. */
	rem_arg = optparse_arg(&options);

	if(rem_arg != NULL)
		cfg->content_filename = SDL_strdup(rem_arg);
	else
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
				"The path to the content file was not given");
		goto err;
	}

	if(cfg->core_filename == NULL)
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

#if ENABLE_VIDEO_RECORDING == 1
struct rec_txt_priv {
	rec_ctx *vid;
	char str[32];
};
char *get_rec_txt(void *priv)
{
	struct rec_txt_priv *rtxt = priv;
	struct {
		Sint64 (*get_size)(rec_ctx *);
	} sz_map[2] = {
		{rec_video_size},
		{rec_audio_size}
	};
	/* Technically MiB and GiB. */
	const char prefix_str[5][3] = {
		" B", "KB", "MB", "GB", "TB"
	};
	Uint64 sz = 0;
	Uint8 prefix = 0;
	unsigned i;

	/* If recording has finished, free memory and delete overlay. */
	if(rtxt->vid == NULL)
	{
		SDL_free(priv);
		return NULL;
	}

	for(i = 0; i < (Uint8)SDL_arraysize(sz_map); i++)
		sz += sz_map[i].get_size(rtxt->vid);

	while(sz > 1 * 1024)
	{
		sz >>= (Uint8)10;
		prefix++;
	}

	SDL_snprintf(rtxt->str, sizeof(rtxt->str), "REC %5" SDL_PRIu64 " %.2s",
			sz, prefix_str[prefix]);

	return rtxt->str;
}
#endif

static SDL_atomic_t screenshot_timeout;
static void take_screenshot(SDL_Renderer *rend, struct core_ctx_s *const ctx)
{
	SDL_Surface *surf;

	/* Screenshots limited to 1 every second. Should not be used for
	 * recording game play as a video, as this function is too slow for
	 * that. */
	if(SDL_AtomicGet(&screenshot_timeout) != 0)
		return;

	SDL_AtomicSet(&screenshot_timeout, 1);
	set_atomic_timeout(1024, &screenshot_timeout, 0, "Enable Screenshot");

	surf = util_tex_to_surf(rend, ctx->sdl.core_tex,
				&ctx->sdl.game_frame_res, ctx->env.flip);
	if(surf == NULL)
		goto err;

	rec_single_img(surf, ctx->core_short_name);

out:
	return;

err:
	SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
		    "Could not take screen capture: %s", SDL_GetError());
	goto out;
}

#if ENABLE_VIDEO_RECORDING == 1
void cap_frame(rec_ctx *vid, SDL_Renderer *rend, SDL_Texture *tex,
	       const SDL_Rect *src, SDL_RendererFlip flip)
{
	SDL_Surface *surf = util_tex_to_surf(rend, tex, src, flip);

	if(surf == NULL)
		return;

	rec_enc_video(vid, surf);
}
#endif

static void handle_rec_toggle(struct haiyajan_ctx_s *ctx)
{
	if(ctx->core.vid == NULL &&
			ctx->core.env.status.bits.valid_frame)
	{
		char vidfile[64];
		SDL_Colour c = { 0x00, 0xFF, 0x00, SDL_ALPHA_OPAQUE };
		struct rec_txt_priv *rtxt;
		gen_filename(vidfile, ctx->core.core_short_name, "h264");

		/* FIXME: add double to Sint32 sample
		 * compensation should the sample rate
		 * not be an integer. */
		ctx->core.vid = rec_init(vidfile,
				ctx->core.sdl.game_frame_res.w,
				ctx->core.sdl.game_frame_res.h,
				ctx->core.av_info.timing.fps,
				SDL_ceil(ctx->core.av_info.timing.sample_rate));
		if(ctx->core.vid == NULL)
		{
			SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO,
					"Unable to initialise libx264: %s",
					SDL_GetError());
			c.r = 0xFF;
			c.g = 0x00;
			c.b = 0x00;
			ui_add_overlay(&ctx->ui_overlay, c,
					ui_overlay_bot_right,
					"Unable to start recording: libx264 failure",
					127, NULL, NULL);
			return;
		}

		SDL_LogInfo(SDL_LOG_CATEGORY_VIDEO, "Video recording started");
		rtxt = SDL_malloc(sizeof(struct rec_txt_priv));
		if(rtxt == NULL)
			goto out;

		rtxt->vid = ctx->core.vid;
		ui_add_overlay(&ctx->ui_overlay, c, ui_overlay_bot_right, NULL,
				0, get_rec_txt, rtxt);
	}
	else if(ctx->core.vid != NULL)
	{
		rec_end(&ctx->core.vid);
	}

out:
	return;
}

static void process_events(struct haiyajan_ctx_s *ctx)
{
	SDL_Event ev;

	while(SDL_PollEvent(&ev) != 0)
	{
		if(ev.type == SDL_QUIT)
		{
			ctx->quit = 1;
			return;
		}
		else if(INPUT_EVENT_CHK(ev.type))
			input_handle_event(&ctx->core.inp, &ev);
		else if(ev.type == ctx->core.inp.input_cmd_event &&
				!ctx->stngs.benchmark)
		{
			switch(ev.user.code)
			{
#if 0
			case INPUT_EVENT_TOGGLE_INFO:
				ctx->stngs.vid_info = !ctx->stngs.vid_info;
				SDL_SetRenderDrawBlendMode(ctx->rend,
						ctx->stngs.vid_info ?
						SDL_BLENDMODE_BLEND :
						SDL_BLENDMODE_NONE);
				if(!ctx->stngs.vid_info)
					break;

				/* Reset FPS counter. */
				fps_beg = SDL_GetTicks();
				fps_curr_frame_dur = fps_calc_frame_dur;
				break;
#endif
			case INPUT_EVENT_TOGGLE_FULLSCREEN:
				ctx->stngs.fullscreen = !ctx->stngs.fullscreen;
				if(ctx->stngs.fullscreen)
				{
					SDL_SetWindowFullscreen(ctx->win,
							SDL_WINDOW_FULLSCREEN_DESKTOP);
				}
				else
				{
					SDL_SetWindowFullscreen(ctx->win, 0);
				}
				break;

			case INPUT_EVENT_TAKE_SCREENSHOT:
			{
				SDL_Colour c = { 0, 0xFF, 0, 0xFF };
				take_screenshot(ctx->rend, &ctx->core);
				ui_add_overlay(&ctx->ui_overlay, c,
						ui_overlay_top_right,
						"SCREENSHOT", 255, NULL, NULL);
				break;
			}
#if ENABLE_VIDEO_RECORDING == 1
			case INPUT_EVENT_RECORD_VIDEO_TOGGLE:
				handle_rec_toggle(ctx);
				break;
#endif
		}
		}
		else if(ev.type == ctx->core.tim.timer_event)
		{
			switch(ev.user.code)
			{
			case TIMER_SPEED_UP_AGGRESSIVELY:
#if ENABLE_VIDEO_RECORDING == 1
				if(ctx->core.vid != NULL)
				{
					rec_speedup(ctx->core.vid);
					rec_speedup(ctx->core.vid);
					rec_speedup(ctx->core.vid);
					rec_speedup(ctx->core.vid);
				}
#endif
				break;

			case TIMER_SPEED_UP:
#if ENABLE_VIDEO_RECORDING == 1
				if(ctx->core.vid != NULL)
					rec_speedup(ctx->core.vid);
#endif
				break;

			case TIMER_OKAY:
			default:
#if ENABLE_VIDEO_RECORDING == 1
				if(ctx->core.vid != NULL)
					rec_relax(ctx->core.vid);
#endif
				break;
			}
		}
	}
}

#if 0
static void run(SDL_Renderer *rend, struct core_ctx_s *ctx)
{
	Uint32 ticks_before, ticks_next, delta_ticks;
	int tim_cmd;
	float fps = 0.0F;
	Uint32 fps_beg;
	const uint_fast8_t fps_calc_frame_dur = 64;
	uint_fast8_t fps_curr_frame_dur = fps_calc_frame_dur;
	Uint32 benchmark_beg;
	const Uint8 frame_skip_max = 4;
	Uint8 frame_skip_count = 4;

#if 0
	{
		Uint32 lim = SDL_GetTicks();
		while(ctx->env.status.bits.valid_frame == 0)
		{
			play_frame(ctx);

			/* If the first frame isn't drawn within two seconds,
			 * break and begin showing screen anyway. */
			if(SDL_GetTicks() - lim > 2000U)
				break;
		}
	}
#endif

	while(ctx->env.status.bits.shutdown == 0)
	{
		/* If in benchmark mode, run as fast as possible. */
		if(ctx->stngs.benchmark)
			tim_cmd = 0;

#if ENABLE_VIDEO_RECORDING == 1
		if(tim_cmd < 0 && ctx->vid == NULL)
#else
			if(tim_cmd < 0)
#endif
		{
			/* Disable video for the skipped frame to improve
			 * performance. But only when we're not recording a
			 * video. */
			ctx->env.status.bits.video_disabled = 1;

		}
		else if(tim_cmd > 0)
			SDL_Delay(tim_cmd);

		timer_profile_start(&tim);
		SDL_SetRenderDrawColor(rend, 0x00, 0x00, 0x00, 0x00);
		SDL_RenderClear(rend);
		play_frame(ctx);
		SDL_RenderCopyEx(rend, ctx->sdl.core_tex,
				 &ctx->sdl.game_frame_res, NULL, 0.0, NULL,
				 ctx->env.flip);

		if(ctx->stngs.vid_info)
		{
			static char busy_str[10] = {'\0'};
			static char fps_str[10] = {'\0'};
			static char acc_str[10] = {'\0'};
			static char aud_str[10] = {'\0'};
			static char frames_str[10] = {'\0'};
			const SDL_Rect font_bg =
				{
					.w = 10 * FONT_CHAR_WIDTH,
					.h = 5 * (FONT_CHAR_HEIGHT + 1),
					.x = 0,
					.y = 0
				};
			SDL_Rect dim = {.w = 1, .h = 1, .x = 0, .y = 0};
			Uint32 ticks_busy = SDL_GetTicks();
			Uint32 busy_diff = ticks_busy - ticks_before;

			/* Only update every five frames to make values
			 * readable. */
			if(fps_curr_frame_dur % 5 == 0)
			{
				SDL_snprintf(busy_str, 10, "%6u ms", busy_diff);
				SDL_snprintf(acc_str, 10, "%6.2f ms",
					     tim.timer_accumulator);
				SDL_snprintf(aud_str, 10, "%6u",
					     (unsigned)(
						     SDL_GetQueuedAudioSize(
							     ctx->sdl.audio_dev)
						     /
						     sizeof(Uint16)
						     / 2U));
				SDL_snprintf(frames_str, 10, "%6u", frames);
			}

			/* Update only after FPS has been recalculated. */
			if(fps_curr_frame_dur == fps_calc_frame_dur)
				SDL_snprintf(fps_str, 10, "%6.2f Hz", fps);

			SDL_SetRenderDrawColor(rend,
					       0x00, 0x00, 0x00, 0x40);
			SDL_RenderFillRect(rend, &font_bg);

			SDL_SetRenderDrawColor(rend,
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

			SDL_SetRenderDrawColor(rend,
					       0x00, 0x00, 0x00, 0x00);
		}

#if ENABLE_VIDEO_RECORDING == 1
		while(ctx->vid != NULL)
		{
			SDL_Rect loc =
				{
					.x = 0,
					.y = FONT_CHAR_WIDTH * 2,
					.w = 2,
					.h = 2
				};
			struct {
				Sint64 (*get_size)(rec_ctx *);

				char str[16];
			} sz_map[2] =
				{
					{rec_video_size, ""},
					{rec_audio_size, ""}
				};

			SDL_RenderGetLogicalSize(rend, &loc.x, NULL);
			if(loc.x < 320)
				break;

			cap_frame(ctx->vid, rend, ctx->sdl.core_tex,
				  &ctx->sdl.game_frame_res, ctx->env.flip);

			SDL_SetRenderDrawColor(rend, UINT8_MAX, 0, 0,
					       SDL_ALPHA_OPAQUE);

			loc.x -= FONT_CHAR_WIDTH * loc.h * 5;
			FontPrintToRenderer(font, " REC", &loc);

			/* Print output file sizes so far. */
			loc.y += FONT_CHAR_HEIGHT * 2;
			loc.w = 1;
			loc.h = 1;

			for(uint_fast8_t i = 0;
			    i < (uint_fast8_t)SDL_arraysize(sz_map); i++)
			{
				/* Technically MiB and GiB. */
				const char prefix[5][3] =
					{
						" B", "KB", "MB", "GB", "TB"
					};
				Uint64 sz = sz_map[i].get_size(ctx->vid);
				Uint8 p = 0;

				while(sz > 1 * 1024)
				{
					sz >>= (Uint8)10;
					p++;
				}

				SDL_snprintf(sz_map[i].str,
					     SDL_arraysize(sz_map[0].str),
					     "%5" PRIu64 " %.2s",
					     sz, prefix[p]);
			}

			FontPrintToRenderer(font, sz_map[0].str, &loc);
			loc.y += FONT_CHAR_HEIGHT;
			FontPrintToRenderer(font, sz_map[1].str, &loc);

			SDL_SetRenderDrawColor(rend,
					       0x00, 0x00, 0x00, 0x00);
			break;
		}
#endif

		/* If the user took a screen capture within the last second,
		 * respond in the interface. */
		if(SDL_AtomicGet(&screenshot_timeout) != 0)
		{
			SDL_Rect loc =
				{
					.x = 0,
					.y = FONT_CHAR_WIDTH * 2,
					.w = 2,
					.h = 2
				};
			SDL_SetRenderDrawColor(rend, UINT8_MAX,
					       UINT8_MAX, 0, SDL_ALPHA_OPAQUE);
			SDL_RenderGetLogicalSize(rend, &loc.x, NULL);
			loc.x -= FONT_CHAR_WIDTH * loc.h * 8;
			FontPrintToRenderer(font, "CAP", &loc);
			SDL_SetRenderDrawColor(rend,
					       0x00, 0x00, 0x00, 0x00);
		}

		timer_profile_end(&tim);

		/* Only draw to screen if we're not falling behind. */
		if(tim_cmd >= 0 || frame_skip_count == 0)
		{
			SDL_RenderPresent(rend);
			frame_skip_count = frame_skip_max;
		}
		else
			frame_skip_count--;

		/* Reset video_disabled for next frame. */
		ctx->env.status.bits.video_disabled = 0;

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
			fps = 1000.0F / ((float)fps_delta
					 / (float)fps_calc_frame_dur);
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
	util_exit_all();
#if ENABLE_VIDEO_RECORDING == 1
	rec_end(&ctx->vid);
#endif
	FontExit(font);
}
#endif

struct benchmark_txt_priv {
	Uint32 fps;
	char str[64];
};
char *get_benchmark_txt(void *priv)
{
	struct benchmark_txt_priv *btxt = priv;
	SDL_snprintf(btxt->str, sizeof(btxt->str), "Benchmark: %u FPS",
			btxt->fps);
	return btxt->str;
}

int haiyajan_get_available_file_types(struct core_ctx_s *ctx)
{
	(void) ctx;
	return 0;
}

static int haiyajan_init_core(struct haiyajan_ctx_s *h,
		char *core_filename,
		char *content_filename)
{
	struct core_ctx_s *ctx = &h->core;

	ctx->core_filename = core_filename;
	ctx->content_filename = content_filename;

	if(load_libretro_core(ctx->core_filename, ctx))
		goto err;

	/* TODO:
	 * - Check that input file is supported by core
	 */
	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
		    "Libretro core \"%.32s\" loaded successfully.",
		    ctx->sys_info.library_name);

#if SDL_VERSION_ATLEAST(2, 0, 13)
	SDL_SetHint(SDL_HINT_AUDIO_DEVICE_STREAM_NAME, ctx->sys_info.library_name);
#endif

	play_init_cb(ctx);

	ctx->sdl.gl = gl_prepare(h->rend);

	if(load_libretro_file(ctx) != 0)
		goto err;

	if(play_init_av(ctx, h->rend) != 0)
		goto err;

	return 0;

err:
	return -1;
}

int main(int argc, char *argv[])
{
	int ret = EXIT_FAILURE;
	struct haiyajan_ctx_s h = {0};

	/* Ignore argc being unused warning. */
	(void)argc;

	SDL_SetMainReady();

#ifdef _WIN32
	/* Windows (MinGW) does not unbuffer stderr by default. */
	setvbuf(stderr, NULL, _IOLBF, BUFSIZ);
#endif

	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_INFO);
	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
		    "%s Libretro Interface -- %d.%d (GIT %s)\n", PROG_NAME,
		    REL_VERSION_MAJOR, REL_VERSION_MINOR, GIT_VERSION);

	init_sig(&h);
	print_info();
	prerun_checks();

#if SDL_VERSION_ATLEAST(2, 0, 13)
	SDL_SetHint(SDL_HINT_AUDIO_DEVICE_APP_NAME, PROG_NAME);
#endif

	if(SDL_Init(SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER
		    | SDL_INIT_TIMER) != 0)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
				"SDL initialisation failed: %s",
				SDL_GetError());
		exit(EXIT_FAILURE);
	}

	apply_settings(argv, &h.stngs);
	h.win = SDL_CreateWindow(PROG_NAME, SDL_WINDOWPOS_UNDEFINED,
				   SDL_WINDOWPOS_UNDEFINED, 320, 240,
				   SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if(h.win == NULL)
		goto err;

	{
		Uint32 flags =
			SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE;
		if(!h.stngs.benchmark)
			flags |= SDL_RENDERER_PRESENTVSYNC;

		h.rend = SDL_CreateRenderer(h.win, -1, flags);
	}
	if(h.rend == NULL)
		goto err;

#if 1
	//SDL_SetHint(SDL_HINT_RENDER_OPENGL_SHADERS, "0");
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
			    SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif

	/* Allows for transparency on information display. */
	if(h.stngs.vid_info)
		SDL_SetRenderDrawBlendMode(h.rend, SDL_BLENDMODE_BLEND);

	SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION,
		       "Created window and renderer");

	h.ui = ui_init(h.rend);
	if(haiyajan_init_core(&h, h.stngs.core_filename,
				h.stngs.content_filename) != 0)
	{
		goto err;
	}
	else
	{
		char title[MAX_TITLE_LEN];
		SDL_snprintf(title, MAX_TITLE_LEN, "%s: %s", PROG_NAME,
			     h.core.sys_info.library_name);
		SDL_SetWindowTitle(h.win, title);
		SDL_SetWindowMinimumSize(h.win, h.core.sdl.game_max_res.w,
				h.core.sdl.game_max_res.h);
		SDL_SetWindowSize(h.win, h.core.sdl.game_max_res.w,
				h.core.sdl.game_max_res.h);
		SDL_RenderSetLogicalSize(h.rend, h.core.sdl.game_max_res.w,
				h.core.sdl.game_max_res.h);
	}

	input_init(&h.core.inp);

	/* TODO: Add return check. */
	timer_init(&h.core.tim, h.core.av_info.timing.fps);
#if 0
	ticks_before = fps_beg = benchmark_beg = SDL_GetTicks();
#endif

	SDL_SetRenderDrawBlendMode(h.rend, SDL_BLENDMODE_BLEND);

	h.font = FontStartup(h.rend);

	while(h.core.env.status.bits.shutdown == 0 && h.quit == 0)
	{
		static int tim_cmd = 0;

		timer_profile_start(&h.core.tim);
		if(tim_cmd > 0)
			SDL_Delay(tim_cmd);

		h.core.env.frames++;
		process_events(&h);
		SDL_SetRenderDrawColor(h.rend, 0x00, 0x00, 0x00, 0x00);
		SDL_RenderClear(h.rend);
		play_frame(&h.core);
		SDL_RenderCopyEx(h.rend, h.core.sdl.core_tex,
				 &h.core.sdl.game_frame_res, NULL, 0.0, NULL,
				 h.core.env.flip);

#if ENABLE_VIDEO_RECORDING == 1
		if(h.core.vid != NULL)
		{
			cap_frame(h.core.vid, h.rend, h.core.sdl.core_tex,
				  &h.core.sdl.game_frame_res, h.core.env.flip);
		}
#endif
		SDL_SetRenderTarget(h.rend, NULL);
		ui_overlay_render(&h.ui_overlay, h.rend, h.font);

		/* Only draw to screen if we're not falling behind. */
		if(tim_cmd >= 0)
			SDL_RenderPresent(h.rend);

		tim_cmd = timer_profile_end(&h.core.tim);

		if(h.stngs.benchmark)
		{
			Uint32 elapsed;
			float bench_fps;
			static Uint32 bench_frames = 0;
			static Uint32 benchmark_beg = 0;
			static struct benchmark_txt_priv btxt;

			/* Run as fast as possible. */
			tim_cmd = 0;

			if(benchmark_beg == 0)
			{
				SDL_Colour c = { 0xFF, 0x00, 0x00, SDL_ALPHA_OPAQUE };
				ui_add_overlay(&h.ui_overlay, c, ui_overlay_top_right, NULL,
						0, get_benchmark_txt,
						&btxt);
				benchmark_beg = SDL_GetTicks();
			}

			bench_frames++;
			elapsed = SDL_GetTicks() - benchmark_beg;
			if(elapsed == 0)
				continue;

			{
				Uint64 bfe4 = bench_frames * 1024;
				btxt.fps = bfe4 / elapsed;
			}

			if(elapsed >= h.stngs.benchmark_dur * 1000)
			{
				bench_fps = (float)bench_frames /
					((float)elapsed / 1000.0F);
				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
						"Benchmark: %.2f FPS",
						bench_fps);
				break;
			}
		}
	}

#if ENABLE_VIDEO_RECORDING == 1
	rec_end(&h.core.vid);
#endif
	util_exit_all();
	ret = EXIT_SUCCESS;

out:
	/* TODO: Free UI.*/

	if(h.core.env.status.bits.game_loaded)
		unload_libretro_file(&h.core);

	if(h.core.env.status.bits.core_init)
	{
		unload_libretro_core(&h.core);
		play_deinit_cb(&h.core);
	}

	SDL_DestroyRenderer(h.rend);
	SDL_DestroyWindow(h.win);
	SDL_VideoQuit();
	SDL_Quit();
	free_settings(&h.core);

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
