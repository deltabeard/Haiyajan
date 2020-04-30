/**
 * Libretro player.
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

#include <libretro.h>
#include <haiyajan.h>
#include <play.h>
#include <input.h>

#define NUM_ELEMS(x) (sizeof(x) / sizeof(*x))

static struct core_ctx_s *ctx_retro = NULL;

/* Forward declarations. */
static uint_fast8_t play_reinit_texture(struct core_ctx_s *c,
	const Uint32 *req_format,
	const unsigned int *req_width,
	const unsigned int *req_height);

void play_frame(struct core_ctx_s *ctx)
{
	ctx->env.status_bits.running = 1;
	ctx->fn.retro_run();
	ctx->env.status_bits.running = 0;
}

/**
 * Converts libretro logging to SDL2 logging.
 */
void play_libretro_log(enum retro_log_level level, const char *fmt, ...)
{
	va_list ap;
	SDL_LogPriority priority;
	char buf[128];

	if(level > RETRO_LOG_ERROR)
		return;

	/* Map libretro priorities to SDL log priorities. */
	priority = level + 2;

	va_start(ap, fmt);

	if(SDL_vsnprintf(buf, sizeof(buf), fmt, ap) < 0)
		SDL_strlcpy(buf, "Unknown log message\n", sizeof(buf));

	va_end(ap);

	SDL_LogMessage(PLAY_LOG_CATEGORY_CORE, priority,
		"%.*s: %s",
		(int)sizeof(ctx_retro->core_log_name),
		ctx_retro->core_log_name, buf);
}

bool cb_retro_environment(unsigned cmd, void *data)
{
	SDL_assert_release(ctx_retro != NULL);

	switch(cmd)
	{
	case RETRO_ENVIRONMENT_SHUTDOWN:
		ctx_retro->env.status_bits.shutdown = 1;
		break;

	case RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL:
	{
		unsigned *perf = data;

		/* Check that this is called in retro_load_game(). */
		/* Abort if this a paranoid debug build. */
		SDL_assert_paranoid(ctx_retro->env.status_bits.core_init == 1);
		SDL_assert_paranoid(ctx_retro->env.status_bits.game_loaded ==
			0);

		SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION,
			"Set performance level to %u", *perf);

		ctx_retro->env.perf_lvl = *perf;
		break;
	}

	/* FIXME: Set this to something better. */
	case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
	{
		/* FIXME: Fix memory leak. */
		const char **sys_dir = data;
		*sys_dir = SDL_GetBasePath();

		if(*sys_dir == NULL)
			return false;

		break;
	}

	case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
	{
		enum retro_pixel_format *fmt = data;
		const Uint32 fmt_tran[] = { SDL_PIXELFORMAT_RGB555,
				SDL_PIXELFORMAT_RGB888,
				SDL_PIXELFORMAT_RGB565
			};

		/* Check that the game hasn't called retro_run() yet. */
		SDL_assert_paranoid(ctx_retro->env.status_bits.running == 0);

		if(*fmt >= NUM_ELEMS(fmt_tran))
		{
			SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO,
				"Invalid format requested from core.");
			return false;
		}

		if(ctx_retro->env.status_bits.running != 0)
		{
			SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
				"Pixel format change was requested, but "
				"not from within retro_run().");
			return false;
		}

		if(play_reinit_texture(ctx_retro, &fmt_tran[*fmt], NULL,
				NULL) != 0)
			return false;

		SDL_LogVerbose(
			SDL_LOG_CATEGORY_APPLICATION,
			"Core request for pixel format %s was accepted",
			SDL_GetPixelFormatName(ctx_retro->env.pixel_fmt));
		break;
	}

#if 0

	case RETRO_ENVIRONMENT_SET_AUDIO_CALLBACK:
	{
		struct retro_audio_callback *audio_cb = data;
		ctx_retro->env.audio_cb = *audio_cb;
		break;
	}

#endif

	case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
	{
		struct retro_log_callback *log_cb = data;
		log_cb->log = play_libretro_log;
		break;
	}

	case RETRO_ENVIRONMENT_SET_GEOMETRY:
	{
		const struct retro_game_geometry *geo = data;

		SDL_assert_paranoid(geo->base_height <=
			ctx_retro->av_info.geometry.max_height);
		SDL_assert_paranoid(geo->base_width <=
			ctx_retro->av_info.geometry.max_width);

		ctx_retro->av_info.geometry.aspect_ratio = geo->aspect_ratio;

		if(play_reinit_texture(ctx_retro, NULL, &geo->base_width,
				&geo->base_width) != 0)
			return false;

		SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION,
			"Modified geometry to %u*%u (%.1f)",
			geo->base_width, geo->base_height,
			geo->aspect_ratio);
		break;
	}

	default:
		return false;
	}

	return true;
}

void cb_retro_video_refresh(const void *data, unsigned width, unsigned height,
	size_t pitch)
{
	SDL_assert(width <= ctx_retro->av_info.geometry.max_width);
	SDL_assert(height <= ctx_retro->av_info.geometry.max_height);
	SDL_assert_paranoid(width <= ctx_retro->av_info.geometry.base_width);
	SDL_assert_paranoid(height <= ctx_retro->av_info.geometry.base_height);
	// SDL_Rect rect = { .w = width, .h = height, .x = 0, .y = 0 };

#if SDL_ASSERT_LEVEL == 3
	size_t tex_pitch;
	int tex_w, tex_h;
	Uint32 format;

	SDL_QueryTexture(ctx_retro->core_tex, &format, NULL, &tex_w,
		&tex_h);
	tex_pitch = tex_w * SDL_BYTESPERPIXEL(format);

	SDL_assert_paranoid(pitch <= tex_pitch);
	SDL_assert_paranoid(format == ctx_retro->env.pixel_fmt);
#endif

	if(SDL_UpdateTexture(ctx_retro->core_tex, NULL, data, pitch) != 0)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
			"Texture could not updated: %s",
			SDL_GetError());
	}

	return;
}

void cb_retro_audio_sample(int16_t left, int16_t right)
{
	(void)left;
	(void)right;
	return;
}

size_t cb_retro_audio_sample_batch(const int16_t *data, size_t frames)
{
	if(ctx_retro->audio_dev == 0)
		goto out;

	/* If the audio driver is lagging too far behind, reset the queue. */
	if(SDL_GetQueuedAudioSize(ctx_retro->audio_dev) >= 32768UL)
		SDL_ClearQueuedAudio(ctx_retro->audio_dev);

	SDL_QueueAudio(ctx_retro->audio_dev, data, frames * sizeof(Uint16) * 2);

out:
	return frames;
}

void cb_retro_input_poll(void)
{
	return;
}

int16_t cb_retro_input_state(unsigned port, unsigned device, unsigned index,
	unsigned id)
{
	return input_get(&ctx_retro->in_ctx, port, device, index, id);
}

static uint_fast8_t play_reinit_texture(struct core_ctx_s *ctx,
	const Uint32 *req_format,
	const unsigned int *req_width,
	const unsigned int *req_height)
{
	float aspect;
	SDL_Texture *test_texture;
	Uint32 format;
	unsigned width;
	unsigned height;

	if(ctx->env.status_bits.game_loaded == 0)
	{
		SDL_LogVerbose(SDL_LOG_CATEGORY_VIDEO, "Not initialising video "
			"until game is loaded.");
		ctx_retro->env.pixel_fmt = req_format != NULL ?
			*req_format : SDL_PIXELFORMAT_RGB555;
		return 0;
	}

	/* Only initialise video if the core hasn't requested it earlier. */
	if(ctx->core_tex == NULL)
		ctx->fn.retro_get_system_av_info(&ctx->av_info);

	format = req_format != NULL ? *req_format : ctx->env.pixel_fmt;
	width = req_width != NULL ? *req_width
		: ctx->av_info.geometry.base_width;
	height = req_height != NULL ? *req_height
		: ctx->av_info.geometry.base_height;

	test_texture =
		SDL_CreateTexture(ctx->disp_rend, format,
			SDL_TEXTUREACCESS_STREAMING, width, height);

	if(test_texture == NULL)
	{
		SDL_LogInfo(SDL_LOG_CATEGORY_VIDEO,
			"Unable to create texture for the requested "
			"format %s: %s",
			SDL_GetPixelFormatName(format), SDL_GetError());
		return 1;
	}

	/* If we have previously created a texture, destroy it and assign the
	 * newly created texture to it. */
	if(ctx->core_tex != NULL)
		SDL_DestroyTexture(ctx->core_tex);

	ctx->core_tex = test_texture;
	ctx->env.pixel_fmt = format;
	ctx->av_info.geometry.base_width = width;
	ctx->av_info.geometry.base_height = height;

	aspect = ctx->av_info.geometry.aspect_ratio;

	if(aspect <= 0.0)
	{
		ctx->game_logical_res.w = ctx->av_info.geometry.base_width;
		ctx->game_logical_res.h = ctx->av_info.geometry.base_height;
	}
	else if(ctx->av_info.geometry.base_height >
		ctx->av_info.geometry.base_width)
	{
		ctx->game_logical_res.w = ctx->av_info.geometry.base_width;
		ctx->game_logical_res.h = SDL_ceilf(
				(float)ctx->av_info.geometry.base_width / aspect);
	}
	else
	{
		ctx->game_logical_res.w =
			SDL_ceilf(ctx->av_info.geometry.base_height * aspect);
		ctx->game_logical_res.h = ctx->av_info.geometry.base_height;
	}

	SDL_LogVerbose(SDL_LOG_CATEGORY_VIDEO, "Created texture: %s %d*%d",
		SDL_GetPixelFormatName(format), width, height);

	return 0;
}

uint_fast8_t play_init_av(struct core_ctx_s *ctx)
{
	SDL_AudioSpec want = { 0 };

	SDL_assert(ctx->env.status_bits.core_init == 1);
	SDL_assert(ctx->env.status_bits.shutdown == 0);
	SDL_assert(ctx->env.status_bits.game_loaded == 1);

	/* If texture is already initialised, don't recreate it. */
	if(ctx->core_tex == NULL)
	{
		ctx->fn.retro_get_system_av_info(&ctx->av_info);
		SDL_LogVerbose(SDL_LOG_CATEGORY_VIDEO,
			"Core is requesting %.2f FPS, %.0f Hz, "
			"%u*%u, %u*%u, %.1f ratio",
			ctx->av_info.timing.fps,
			ctx->av_info.timing.sample_rate,
			ctx->av_info.geometry.base_width,
			ctx->av_info.geometry.base_height,
			ctx->av_info.geometry.max_width,
			ctx->av_info.geometry.max_height,
			ctx->av_info.geometry.aspect_ratio);

		if(play_reinit_texture(ctx, &ctx->env.pixel_fmt, NULL, NULL) !=
			0)
		{
			SDL_SetError("Unable to create texture: %s",
				SDL_GetError());
			return 1;
		}
	}

	want.freq = ctx->av_info.timing.sample_rate;
	want.format = AUDIO_S16SYS;
	want.channels = 2;
	want.samples = 512;
	want.callback = NULL;

	ctx->audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, NULL, 0);

	if(ctx->audio_dev == 0)
	{
		SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO, "Failed to open audio: %s",
			SDL_GetError());
	}
	else
	{
		SDL_LogInfo(SDL_LOG_CATEGORY_AUDIO,
			"Audio driver %s initialised",
			SDL_GetCurrentAudioDriver());
		SDL_PauseAudioDevice(ctx->audio_dev, 0);
	}

	return 0;
}

void play_init_cb(struct core_ctx_s *ctx)
{
	SDL_assert_paranoid(ctx != NULL);

	ctx_retro = ctx;

	/* Set log name for core. */
	{
		size_t len;
		int printed;
		char *c = SDL_strchr(ctx->sys_info.library_name, ' ');

		if(c == NULL)
			len = SDL_strlen(ctx->sys_info.library_name);
		else
			len = c - ctx->sys_info.library_name;

		printed = SDL_snprintf(ctx->core_log_name,
				sizeof(ctx->core_log_name), "%.*s",
				(int)len, ctx->sys_info.library_name);

		if(printed < 0)
		{
			SDL_strlcpy(ctx->core_log_name, "CORE",
				sizeof(ctx->core_log_name));
		}

		while(printed > 0)
		{
			ctx->core_log_name[printed] =
				SDL_toupper(ctx->core_log_name[printed]);
			printed--;
		}
	}

	/* Set default pixel format. */
	ctx->env.pixel_fmt = SDL_PIXELFORMAT_RGB555;
	ctx->core_tex = NULL;

	ctx->fn.retro_set_environment(cb_retro_environment);
	ctx->fn.retro_set_video_refresh(cb_retro_video_refresh);
	ctx->fn.retro_set_audio_sample(cb_retro_audio_sample);
	ctx->fn.retro_set_audio_sample_batch(cb_retro_audio_sample_batch);
	ctx->fn.retro_set_input_poll(cb_retro_input_poll);
	ctx->fn.retro_set_input_state(cb_retro_input_state);

	/* Error in libretro core dev overview: retro_init() should be called
	 * after retro_set_*() functions. */
	ctx->fn.retro_init();

	ctx->env.status_bits.core_init = 1;
}

void play_deinit_cb(struct core_ctx_s *ctx)
{
	if(ctx == NULL)
		return;

	if(ctx->core_tex != NULL)
	{
		SDL_DestroyTexture(ctx->core_tex);
		ctx->core_tex = NULL;
	}

	SDL_CloseAudioDevice(ctx->audio_dev);
	ctx_retro = NULL;
}
