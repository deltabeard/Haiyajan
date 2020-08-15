/**
 * Libretro player.
 * Copyright (C) 2020  Mahyar Koshkouei
 *
 * This is free software, and you are welcome to redistribute it under the terms
 * of the GNU Affero General Public License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 *
 * See the LICENSE file for more details.
 */

#include <SDL.h>

#include <libretro.h>
#include <haiyajan.h>
#include <play.h>
#include <input.h>
#include <rec.h>

#define NUM_ELEMS(x) (sizeof(x) / sizeof(*x))

static struct core_ctx_s *ctx_retro = NULL;

void play_frame(struct core_ctx_s *ctx)
{
	if(ctx->env.status.bits.opengl_required != 0)
		gl_prerun(ctx->sdl.gl);

	if(ctx->env.ftcb != NULL)
		ctx->env.ftcb(ctx->env.ftref);

	ctx->env.status.bits.playing = 1;
	ctx->fn.retro_run();
	ctx->env.status.bits.playing = 0;

	if(ctx->env.status.bits.opengl_required != 0)
		gl_postrun(ctx->sdl.gl);
}

/**
 * Converts libretro logging to SDL2 logging.
 */
void play_libretro_log(enum retro_log_level level, const char *fmt, ...)
{
	va_list ap;
	SDL_LogPriority priority;
	char buf[192];

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
		(int)sizeof(ctx_retro->core_short_name),
		ctx_retro->core_short_name, buf);
}

bool cb_retro_environment(unsigned cmd, void *data)
{
	const Uint8 exp = (cmd >> 4);
	SDL_assert_release(ctx_retro != NULL);

	cmd &= 0xFF;
	switch(cmd)
	{
	case RETRO_ENVIRONMENT_GET_CAN_DUPE:
		/* Passing NULL to the video callback will not update the
		 * texture. */
		break;

	case RETRO_ENVIRONMENT_SHUTDOWN:
		ctx_retro->env.status.bits.shutdown = 1;
		SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION,
			       "Core requested the frontend to shutdown");
		break;

	case RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL:
	{
		unsigned *perf = data;

		/* Check that this is called in retro_load_game(). */
		/* Abort if this a paranoid debug build. */
		SDL_assert_paranoid(ctx_retro->env.status.bits.core_init == 1);
		SDL_assert_paranoid(ctx_retro->env.status.bits.game_loaded == 0);

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

		/* Pixel format must be set before the video display is
		 * initialised. */
		if(ctx_retro->env.status.bits.av_init == 1)
		{
			SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO,
				    "The core's request to change the pixel "
				    "format of the display after video "
				    "initialisation was denied");
			return false;
		}

		if(*fmt >= NUM_ELEMS(fmt_tran))
		{
			SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO,
				"Invalid format requested from core.");
			return false;
		}

		ctx_retro->env.pixel_fmt = fmt_tran[*fmt];

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
	case RETRO_ENVIRONMENT_SET_HW_RENDER:
	{
		struct retro_hw_render_callback *hw_cb = data;
		const char *const ctx_type[] =
		{
			"None", "OpenGL 2.x", "OpenGL ES 2",
			"OpenGL Specific", "OpenGL ES 3",
			"OpenGL ES Specific", "Vulkan", "Direct3D"
		};
		
		SDL_assert(ctx_retro->env.status.bits.game_loaded == 0);

		switch(hw_cb->context_type)
		{
		case RETRO_HW_CONTEXT_OPENGL_CORE:
		case RETRO_HW_CONTEXT_OPENGL:
		case RETRO_HW_CONTEXT_OPENGLES2:
		case RETRO_HW_CONTEXT_OPENGLES3:
		{
			int gl_ret = gl_init(ctx_retro->sdl.gl,
						&ctx_retro->sdl.core_tex, hw_cb);
			if(gl_ret == SDL_FALSE)
			{
				SDL_LogWarn(SDL_LOG_CATEGORY_RENDER,
					"The requested %s context could "
					"not be initialised: %s",
					ctx_type[hw_cb->context_type],
					SDL_GetError());
				return false;
			}

			if(hw_cb->bottom_left_origin)
				ctx_retro->env.flip = SDL_FLIP_VERTICAL;

			SDL_LogInfo(SDL_LOG_CATEGORY_RENDER,
				    "The request for an %s (%u.%u) context was "
				    "accepted",
				    ctx_type[hw_cb->context_type],
				hw_cb->version_major, hw_cb->version_minor);
			ctx_retro->env.status.bits.opengl_required = 1;

			break;
		}

		default:
			SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
						"Hardware context %s (%u.%u) is not supported",
						ctx_type[hw_cb->context_type],
						hw_cb->version_major, hw_cb->version_minor);
			return false;
		}

		break;
	}

	case RETRO_ENVIRONMENT_SET_VARIABLES:
	{
		const struct retro_variable *var = data;
		while(var->key != NULL)
		{
			SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION,
				"Core set variable: %s, %s",
				var->key, var->value);
			var++;
		}
		return false;
	}

	case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
	{
		bool *updated = data;
		*updated = SDL_FALSE;
		break;
	}

	case RETRO_ENVIRONMENT_GET_LIBRETRO_PATH:
	{
		const char **core_path = data;

		/* FIXME: fix memory leak. */
		char *tmp = SDL_strdup(ctx_retro->core_filename);
		//size_t len = SDL_strlen(ctx_retro->file_core);
		*core_path = tmp;

#if 0
		for(char *i = tmp + len; i != tmp; i--)
		{
			if(*i == ctx_retro->path_sep)
			{
				*i = '\0';
				break;
			}
		}
#endif

		break;
	}

	case RETRO_ENVIRONMENT_SET_FRAME_TIME_CALLBACK:
	{
		const struct retro_frame_time_callback *ftcb = data;
		ctx_retro->env.ftcb = ftcb->callback;
		ctx_retro->env.ftref = ftcb->reference;
		SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION,
				"Set frame time callback");
		break;
	}

	case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
	{
		struct retro_log_callback *log_cb = data;
		log_cb->log = play_libretro_log;
		break;
	}

	case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
	{
		const char **save_dir = data;
		/* FIXME: temporary. */
		*save_dir = SDL_GetBasePath();

		if(*save_dir == NULL)
			return false;

		break;
#if 0
		char *base_path;
		size_t base_path_len;
		char path_sep;

		/* Get path separator for current platform. */
		base_path = SDL_GetBasePath();
		if(base_path == NULL)
			return false;

		base_path_len = SDL_strlen(base_path);
		/* SDL2 guarantees that this string ends with a path separator.
		 */
		path_sep = *(base_path + base_path_len - 1);
#endif
	}

	case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:
	{
		const struct retro_controller_info *info = data;
		do {
			unsigned port;
			for(port = 0; port < info->num_types; port++)
			{
				input_add_controller(&ctx_retro->inp, port, info->types[port].id);
			}
		} while((++info)->types != 0);

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

		SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION,
			"Modified geometry to %u*%u (%.1f)",
			geo->base_width, geo->base_height,
			geo->aspect_ratio);
		break;
	}
	case (RETRO_ENVIRONMENT_GET_AUDIO_VIDEO_ENABLE & 0xFF):
	{
		/* An unsigned integer is a better choice than signed for bit
		 * flipping. */
		Uint8 *av_en = data;
		/* Audio is always enabled. */
		const Uint8 audio_disabled = 0;

		*av_en = ((!audio_disabled) << 1) |
			((!ctx_retro->env.status.bits.video_disabled) << 0);
		break;
	}

	case (RETRO_ENVIRONMENT_SET_HW_SHARED_CONTEXT & 0xFF):
	{
		/* Check if RETRO_ENVIRONMENT_SET_SERIALIZATION_QUIRKS */
		if(!exp)
			goto unsupported;

		break;
	}

	case RETRO_ENVIRONMENT_GET_PREFERRED_HW_RENDER:
	{
		unsigned *pref = data;
		*pref = RETRO_HW_CONTEXT_OPENGL;
		break;
	}

	default:
unsupported:
	{
#if 1
		static char log_hist[64] = { 0 };
		if(log_hist[cmd] == 0)
		{
			SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION,
				"Unsupported environment command %u", cmd);
			log_hist[cmd] = 1;
		}
#else
		SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION,
				"Unsupported environment command %u", cmd);
#endif

		return false;
	}
	}
	return true;
}

void cb_retro_video_refresh(const void *data, unsigned width, unsigned height,
	size_t pitch)
{
	ctx_retro->sdl.game_frame_res.h = height;
	ctx_retro->sdl.game_frame_res.w = width;

	if(data == NULL || ctx_retro->env.status.bits.video_disabled)
	{
		ctx_retro->env.status.bits.valid_frame = 0;
		return;
	}

	ctx_retro->env.status.bits.valid_frame = 1;

	if(data == RETRO_HW_FRAME_BUFFER_VALID)
		return;

	SDL_assert(width <= ctx_retro->av_info.geometry.max_width);
	SDL_assert(height <= ctx_retro->av_info.geometry.max_height);

#if SDL_ASSERT_LEVEL == 3
	size_t tex_pitch;
	int tex_w, tex_h;
	Uint32 format;

	SDL_QueryTexture(ctx_retro->sdl.core_tex, &format, NULL, &tex_w,
		&tex_h);
	tex_pitch = tex_w * SDL_BYTESPERPIXEL(format);

	SDL_assert_paranoid(pitch <= tex_pitch);
	SDL_assert_paranoid(format == ctx_retro->env.pixel_fmt);
#endif

	if(ctx_retro->env.status.bits.opengl_required)
		return;

	if(SDL_UpdateTexture(ctx_retro->sdl.core_tex, &ctx_retro->sdl.game_frame_res, data, (int)pitch) != 0)
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
	if(ctx_retro->sdl.audio_dev == 0)
		goto out;

	/* If the audio driver is lagging too far behind, reset the queue. */
	if(SDL_GetQueuedAudioSize(ctx_retro->sdl.audio_dev) >= 32768UL)
		SDL_ClearQueuedAudio(ctx_retro->sdl.audio_dev);

#if ENABLE_VIDEO_RECORDING == 1
	if(ctx_retro->vid != NULL)
	{
		rec_enc_audio(ctx_retro->vid, data, frames);
	}
#endif

	SDL_QueueAudio(ctx_retro->sdl.audio_dev, data, (Uint32)frames * sizeof(Uint16) * 2);

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
	return input_get(&ctx_retro->inp, port, device, index, id);
}

/* TODO: Initialise texture to max width/height. */
static int play_reinit_texture(struct core_ctx_s *ctx,
					SDL_Renderer *rend,
	const Uint32 *req_format,
	const unsigned int *new_max_width,
	const unsigned int *new_max_height)
{
	SDL_Texture *test_texture;
	Uint32 format;
	unsigned width;
	unsigned height;

	format = req_format != NULL ? *req_format : ctx->env.pixel_fmt;
	width = new_max_width != NULL ? *new_max_width
		: ctx->av_info.geometry.max_width;
	height = new_max_height != NULL ? *new_max_height
		: ctx->av_info.geometry.max_height;

	test_texture = SDL_CreateTexture(rend, format,
			ctx->env.status.bits.opengl_required ?
				SDL_TEXTUREACCESS_TARGET :
				SDL_TEXTUREACCESS_STREAMING,
			width, height);

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
	if(ctx->sdl.core_tex != NULL)
		SDL_DestroyTexture(ctx->sdl.core_tex);

	ctx->sdl.core_tex = test_texture;
	ctx->env.pixel_fmt = format;
	ctx->sdl.game_max_res.w = width;
	ctx->sdl.game_max_res.h = height;

	SDL_LogVerbose(SDL_LOG_CATEGORY_VIDEO, "Created texture: %s %d*%d",
		SDL_GetPixelFormatName(format), width, height);

	return 0;
}

int play_init_av(struct core_ctx_s *ctx, SDL_Renderer *rend)
{
	SDL_AudioSpec want = { 0 };

	SDL_assert(ctx->env.status.bits.core_init == 1);
	SDL_assert(ctx->env.status.bits.shutdown == 0);
	SDL_assert(ctx->env.status.bits.game_loaded == 1);
	SDL_assert(ctx->env.status.bits.av_init == 0);

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

	if(play_reinit_texture(ctx, rend, &ctx->env.pixel_fmt,
		&ctx->av_info.geometry.max_width,
		&ctx->av_info.geometry.max_height) != 0)
	{
		SDL_SetError("Unable to create texture: %s",
					 SDL_GetError());
		return 1;
	}

	if(ctx->env.pixel_fmt == 0)
		ctx->env.pixel_fmt = SDL_PIXELFORMAT_RGB888;

	if(ctx->sdl.gl != NULL)
		gl_reset_context(ctx->sdl.gl);

	want.freq = (int)ctx->av_info.timing.sample_rate;
	want.format = AUDIO_S16SYS;
	want.channels = 2;
	want.samples = 512;
	want.callback = NULL;

	ctx->sdl.audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, NULL, 0);

	if(ctx->sdl.audio_dev == 0)
	{
		SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO, "Failed to open audio: %s",
			SDL_GetError());
	}
	else
	{
		SDL_LogInfo(SDL_LOG_CATEGORY_AUDIO,
			"Audio driver %s initialised",
			SDL_GetCurrentAudioDriver());
		SDL_PauseAudioDevice(ctx->sdl.audio_dev, 0);
	}

	ctx->fn.retro_set_controller_port_device(0, RETRO_DEVICE_JOYPAD);
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

		printed = SDL_snprintf(ctx->core_short_name,
				sizeof(ctx->core_short_name), "%.*s",
				(int)len, ctx->sys_info.library_name);

		if(printed < 0)
		{
			SDL_strlcpy(ctx->core_short_name, "CORE",
				sizeof(ctx->core_short_name));
		}

		while(printed >= 0)
		{
			ctx->core_short_name[printed] =
				SDL_toupper(ctx->core_short_name[printed]);
			printed--;
		}
	}

	/* Set default pixel format. */
	if(ctx->env.pixel_fmt == 0)
		ctx->env.pixel_fmt = SDL_PIXELFORMAT_RGB555;

	ctx->fn.retro_set_environment(cb_retro_environment);
	ctx->fn.retro_set_video_refresh(cb_retro_video_refresh);
	ctx->fn.retro_set_audio_sample(cb_retro_audio_sample);
	ctx->fn.retro_set_audio_sample_batch(cb_retro_audio_sample_batch);
	ctx->fn.retro_set_input_poll(cb_retro_input_poll);
	ctx->fn.retro_set_input_state(cb_retro_input_state);

	/* Error in libretro core dev overview: retro_init() should be called
	 * after retro_set_*() functions. */
	ctx->fn.retro_init();
	SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "Core initialised");

	ctx->env.status.bits.core_init = 1;
}

void play_deinit_cb(struct core_ctx_s *ctx)
{
	if(ctx == NULL)
		return;

	gl_deinit(ctx->sdl.gl);

	if(ctx->sdl.core_tex != NULL)
	{
		SDL_DestroyTexture(ctx->sdl.core_tex);
		ctx->sdl.core_tex = NULL;
	}

	SDL_CloseAudioDevice(ctx->sdl.audio_dev);
	ctx_retro = NULL;
}

