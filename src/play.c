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

static struct core_ctx_s *ctx;

void play_set_ctx(struct core_ctx_s *c)
{
	ctx = c;

	/* Set default pixel format. */
	ctx->env.pixel_fmt = SDL_PIXELFORMAT_RGB555;
	ctx->game_texture = NULL;
	ctx->game_pixels = NULL;
	ctx->game_pixels_sz = 0;
}

void play_frame(void)
{
	ctx->fn.retro_run();
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

		if(ctx->env.status_bits.core_init != 1 ||
		   ctx->env.status_bits.game_loaded != 0)
		{
			return false;
		}

		ctx->env.perf_lvl = *perf;
		break;
	}

	case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
	{
		enum retro_pixel_format *fmt = data;
		const Uint32 fmt_tran[] = { SDL_PIXELFORMAT_RGB555,
					    SDL_PIXELFORMAT_RGB24,
					    SDL_PIXELFORMAT_RGB565 };

		/* Check that the game hasn't called retro_run() yet. */
		SDL_assert_paranoid(ctx->env.status_bits.running == 0);
		if(*fmt >= RETRO_PIXEL_FORMAT_UNKNOWN)
			return false;

		if(ctx->env.status_bits.running != 0)
			return false;

		ctx->env.pixel_fmt = fmt_tran[*fmt];

		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
			    "Core request for pixel format %s was accepted",
			    SDL_GetPixelFormatName(ctx->env.pixel_fmt));
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
#if 0
	size_t sz = width * height * SDL_BITSPERPIXEL(ctx->env.pixel_fmt);

	if(sz != ctx->game_pixels_sz && ctx->game_surface != NULL)
	{
		SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "Pixel buffer size mismatch.");
		SDL_FreeSurface(ctx->game_surface);
		free(ctx->game_pixels);
	}

	if(ctx->game_surface == NULL)
	{
		ctx->game_pixels = malloc(sz);
		ctx->game_pixels_sz = sz;

		ctx->game_surface = SDL_CreateRGBSurfaceWithFormatFrom(
			ctx->game_pixels, width, height,
			SDL_BITSPERPIXEL(ctx->env.pixel_fmt),
			pitch,
			ctx->env.pixel_fmt);


		SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION,
			       "Created Surface with pixel format %s, "
			       "and size %zu, %u*%u, pitch %zu",
			       SDL_GetPixelFormatName(ctx->env.pixel_fmt),
			       sz, width, height, pitch);
	}

	memcpy(ctx->game_pixels, data, sz);
#else
	void *pixels;
	int tex_pitch;
	int tex_w, tex_h;
	size_t sz;

	if(ctx->game_texture != NULL)
	{
		SDL_QueryTexture(ctx->game_texture, NULL, NULL, &tex_w, &tex_h);
		if(tex_w != width || tex_h != height)
		{
			SDL_DestroyTexture(ctx->game_texture);
			ctx->game_texture = NULL;
		}
	}

	if(ctx->game_texture == NULL)
	{
		ctx->game_texture = SDL_CreateTexture(ctx->disp_rend,
			ctx->env.pixel_fmt, SDL_TEXTUREACCESS_STREAMING,
			width, height);

		if(ctx->game_texture == NULL)
		{
			/* TODO: Limit logging here .*/
			SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
					"Unable to create texture: %s",
					SDL_GetError());
			return;
		}
	}
	else
	{
		SDL_QueryTexture(ctx->game_texture, NULL, NULL, &tex_w, &tex_h);

	}

	if(SDL_LockTexture(ctx->game_texture, NULL, &pixels, &tex_pitch) != 0)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
				"Texture could not be locked: %s",
				SDL_GetError());
		return;
	}

	sz = (tex_pitch / width) * width * height;
	memcpy(pixels, data, sz);
	SDL_UnlockTexture(ctx->game_texture);
#endif

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
