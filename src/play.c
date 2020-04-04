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
#include <parsley.h>
#include <play.h>

#define NUM_ELEMS(x) (sizeof(x) / sizeof(*x))

static struct core_ctx_s *ctx = NULL;

/* Forward declarations. */
static uint_fast8_t play_reinit_texture(struct core_ctx_s *c,
					const Uint32 *req_format,
					const unsigned int *req_width,
					const unsigned int *req_height);

void play_frame(void)
{
	ctx->env.status_bits.running = 1;
	ctx->fn.retro_run();
	ctx->env.status_bits.running = 0;
}

bool cb_retro_environment(unsigned cmd, void *data)
{
	SDL_assert_release(ctx != NULL);

	switch(cmd)
	{
	case RETRO_ENVIRONMENT_SHUTDOWN:
		ctx->env.status_bits.shutdown = 1;
		break;

	case RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL:
	{
		unsigned *perf = data;

		/* Check that this is called in retro_load_game(). */
		/* Abort if this a paranoid debug build. */
		SDL_assert_paranoid(ctx->env.status_bits.core_init == 1);
		SDL_assert_paranoid(ctx->env.status_bits.game_loaded == 0);

		SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION,
			       "Set performance level to %u", *perf);

		ctx->env.perf_lvl = *perf;
		break;
	}

	case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
	{
		enum retro_pixel_format *fmt = data;
		const Uint32 fmt_tran[] = { SDL_PIXELFORMAT_RGB555,
					    SDL_PIXELFORMAT_RGB888,
					    SDL_PIXELFORMAT_RGB565 };

		/* Check that the game hasn't called retro_run() yet. */
		SDL_assert_paranoid(ctx->env.status_bits.running == 0);

		if(*fmt >= NUM_ELEMS(fmt_tran))
		{
			SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO,
				    "Invalid format requested from core.");
			return false;
		}

		if(ctx->env.status_bits.running != 0)
		{
			SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
				    "Pixel format change was requested, but "
				    "not from within retro_run().");
			return false;
		}

		if(play_reinit_texture(ctx, &fmt_tran[*fmt], NULL, NULL) != 0)
			return false;

		SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION,
			       "Core request for pixel format %s was accepted",
			       SDL_GetPixelFormatName(ctx->env.pixel_fmt));
		break;
	}

	case RETRO_ENVIRONMENT_SET_GEOMETRY:
	{
		const struct retro_game_geometry *geo = data;

		SDL_assert_paranoid(geo->base_height <=
				    ctx->av_info.geometry.max_height);
		SDL_assert_paranoid(geo->base_width <=
				    ctx->av_info.geometry.max_width);

		ctx->av_info.geometry.aspect_ratio = geo->aspect_ratio;

		if(play_reinit_texture(ctx, NULL, geo->base_width, geo->base_width) != 0)
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
	SDL_assert(width <= ctx->av_info.geometry.max_width);
	SDL_assert(height <= ctx->av_info.geometry.max_height);
	SDL_assert_paranoid(width <= ctx->av_info.geometry.base_width);
	SDL_assert_paranoid(height <= ctx->av_info.geometry.base_height);
	// SDL_Rect rect = { .w = width, .h = height, .x = 0, .y = 0 };

#if SDL_ASSERT_LEVEL == 3
	int tex_pitch;
	int tex_w, tex_h;
	Uint32 format;

	SDL_QueryTexture(ctx->game_texture, &format, NULL, &tex_w, &tex_h);
	tex_pitch = tex_w * SDL_BYTESPERPIXEL(format);

	SDL_assert_paranoid(pitch <= tex_pitch);
	SDL_assert_paranoid(format == ctx->env.pixel_fmt);
#endif

	if(SDL_UpdateTexture(ctx->game_texture, NULL, data, pitch) != 0)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
				"Texture could not updated: %s",
				SDL_GetError());
	}

	return;
}

void cb_retro_audio_sample(int16_t left, int16_t right)
{
	return;
}

size_t cb_retro_audio_sample_batch(const int16_t *data, size_t frames)
{
	return 0;
}

void cb_retro_input_poll(void)
{
	return;
}

int16_t cb_retro_input_state(unsigned port, unsigned device, unsigned index,
			     unsigned id)
{
	return 0;
}

static uint_fast8_t play_reinit_texture(struct core_ctx_s *c,
					const Uint32 *req_format,
					const unsigned int *req_width,
					const unsigned int *req_height)
{
	float aspect;
	SDL_Texture *test_texture;
	Uint32 format = req_format != NULL ? *req_format : c->env.pixel_fmt;
	unsigned width = req_width != NULL ? *req_width
					   : c->av_info.geometry.base_width;
	unsigned height = req_height != NULL ? *req_height
					     : c->av_info.geometry.base_height;

	test_texture = SDL_CreateTexture(c->disp_rend, format,
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
	if(c->game_texture != NULL)
		SDL_DestroyTexture(c->game_texture);

	c->game_texture = test_texture;
	c->env.pixel_fmt = format;
	c->av_info.geometry.base_width = width;
	c->av_info.geometry.base_height = height;

	aspect = c->av_info.geometry.aspect_ratio;

	if(aspect <= 0.0)
	{
		c->game_logical_res.w = c->av_info.geometry.base_width;
		c->game_logical_res.h = c->av_info.geometry.base_height;
	}
	else if(c->av_info.geometry.base_height >
		c->av_info.geometry.base_width)
	{
		c->game_logical_res.w = c->av_info.geometry.base_width;
		c->game_logical_res.h = SDL_ceilf(
			(float)c->av_info.geometry.base_width / aspect);
	}
	else
	{
		c->game_logical_res.w =
			SDL_ceilf(c->av_info.geometry.base_height * aspect);
		c->game_logical_res.h = c->av_info.geometry.base_height;
	}

	SDL_LogVerbose(SDL_LOG_CATEGORY_VIDEO, "Created texture: %s %d*%d",
			SDL_GetPixelFormatName(format), width, height);

	return 0;
}

uint_fast8_t play_init_av(void)
{
	SDL_assert(ctx->env.status_bits.core_init == 1);
	SDL_assert(ctx->env.status_bits.shutdown == 0);
	SDL_assert(ctx->env.status_bits.game_loaded == 1);

	ctx->fn.retro_get_system_av_info(&ctx->av_info);
	SDL_LogVerbose(SDL_LOG_CATEGORY_VIDEO,
		       "Core is requesting %.2f FPS, %.0f Hz, "
		       "%u*%u, %u*%u, %.1f ratio",
		       ctx->av_info.timing.fps, ctx->av_info.timing.sample_rate,
		       ctx->av_info.geometry.base_width,
		       ctx->av_info.geometry.base_height,
		       ctx->av_info.geometry.max_width,
		       ctx->av_info.geometry.max_height,
		       ctx->av_info.geometry.aspect_ratio);

	if(play_reinit_texture(ctx, &ctx->env.pixel_fmt, NULL, NULL) != 0)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_VIDEO,
				"Unable to create texture: %s", SDL_GetError());
		return 1;
	}

	return 0;
}

void play_init_cb(struct core_ctx_s *c)
{
	SDL_assert_paranoid(ctx == NULL);

	/* TODO: Move to load.c? */
	ctx = c;

	/* Set default pixel format. */
	ctx->env.pixel_fmt = SDL_PIXELFORMAT_RGB555;
	ctx->game_texture = NULL;

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

void play_deinit_display(void)
{
	if(ctx->game_texture != NULL)
		SDL_DestroyTexture(ctx->game_texture);

	ctx->game_texture = NULL;
	ctx = NULL;
}
