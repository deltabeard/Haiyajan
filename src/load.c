/**
 * Handles the loading of files, including libretro cores and emulator files.
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
#include <stdint.h>

#include <libretro.h>
#include <load.h>

#define NUM_ELEMS(x) (sizeof(x) / sizeof(*x))

uint_fast8_t load_libretro_file(const char *file, struct core_ctx_s *ctx)
{
	/* TODO:
	 * - Check whether file must be loaded into RAM, or is read straight
	 * from disk.
	 * - Read file.
	 *
	 * If need_fullpath in retro_system_info is true, then don't load the
	 * ROM into memory. If false, then Parsley must load the ROM image into
	 * memory.
	 */
	struct retro_game_info game = { .path = file, .meta = NULL };

	SDL_assert_paranoid(ctx != NULL);
	SDL_assert(ctx->env.status_bits.core_init == 1);

	if(ctx->sys_info.need_fullpath == true)
	{
		ctx->game_data = NULL;
		game.data = NULL;
	}
	else
	{
		/* Read file to memory. */
		SDL_RWops *game_file;

		game_file = SDL_RWFromFile(file, "rb");
		if(game_file == NULL)
			return 1;

	        game.size = SDL_RWsize(game_file);
		ctx->game_data = malloc(game.size);
		if(ctx->game_data == NULL)
		{
			SDL_SetError("Unable to allocate memory for game.");
			return 1;
		}

		if(SDL_RWread(game_file, ctx->game_data, game.size, 1) == 0)
		{
			free(ctx->game_data);
			return 1;
		}

		game.data = ctx->game_data;
		SDL_RWclose(game_file);
	}

	if(ctx->fn.retro_load_game(&game) == false)
		return 1;

	ctx->env.status_bits.game_loaded = 1;
	ctx->env.status_bits.shutdown = 0;

	return 0;
}

void unload_libretro_file(struct core_ctx_s *ctx)
{
	free(ctx->game_data);
	ctx->game_data = NULL;
	ctx->env.status_bits.game_loaded = 0;
}

uint_fast8_t load_libretro_core(const char *so_file, struct core_ctx_s *ctx)
{
	struct fn_links_s
	{
		/* clang-format off */
		/* Name of libretro core function. */
		const char *fn_str;

		/* The following is a lot of bloat to appease a compiler warning
		 * about assigning void pointers to function pointers. */
		union {
			void **sdl_fn;

			void (**retro_init)(void);
			void (**retro_deinit)(void);
			unsigned (**retro_api_version)(void);

			void (**retro_set_environment)(retro_environment_t);
			void (**retro_set_video_refresh)(retro_video_refresh_t);
			void (**retro_set_audio_sample)(retro_audio_sample_t);
			void (**retro_set_audio_sample_batch)(retro_audio_sample_batch_t);
			void (**retro_set_input_poll)(retro_input_poll_t);
			void (**retro_set_input_state)(retro_input_state_t);

			void (**retro_get_system_info)(struct retro_system_info *info);
			void (**retro_get_system_av_info)(struct retro_system_av_info *info);
			void (**retro_set_controller_port_device)(unsigned port, unsigned device);

			void (**retro_reset)(void);
			void (**retro_run)(void);
			size_t (**retro_serialize_size)(void);
			bool (**retro_serialize)(void *data, size_t size);
			bool (**retro_unserialize)(const void *data, size_t size);

			void (**retro_cheat_reset)(void);
			void (**retro_cheat_set)(unsigned index, bool enabled, const char *code);
			bool (**retro_load_game)(const struct retro_game_info *game);
			bool (**retro_load_game_special)(unsigned game_type, const struct retro_game_info *info, size_t num_info);
			void (**retro_unload_game)(void);
			unsigned (**retro_get_region)(void);

			void *(**retro_get_memory_data)(unsigned id);
			size_t (**retro_get_memory_size)(unsigned id);
		} fn_ptr;
	} const fn_links[] = {
		{ "retro_init",			{ .retro_init = &ctx->fn.retro_init } },
		{ "retro_deinit",		{ .retro_init = &ctx->fn.retro_deinit } },
		{ "retro_api_version",		{ .retro_api_version = &ctx->fn.retro_api_version } },

		{ "retro_set_environment",	{ .retro_set_environment = &ctx->fn.retro_set_environment } },
		{ "retro_set_video_refresh",	{ .retro_set_video_refresh = &ctx->fn.retro_set_video_refresh } },
		{ "retro_set_audio_sample",	{ .retro_set_audio_sample = &ctx->fn.retro_set_audio_sample } },
		{ "retro_set_audio_sample_batch", { .retro_set_audio_sample_batch = &ctx->fn.retro_set_audio_sample_batch } },
		{ "retro_set_input_poll",	{ .retro_set_input_poll = &ctx->fn.retro_set_input_poll } },
		{ "retro_set_input_state",	{ .retro_set_input_state = &ctx->fn.retro_set_input_state } },

		{ "retro_get_system_info",	{ .retro_get_system_info = &ctx->fn.retro_get_system_info } },
		{ "retro_get_system_av_info",	{ .retro_get_system_av_info = &ctx->fn.retro_get_system_av_info } },
		{ "retro_set_controller_port_device", { .retro_set_controller_port_device = &ctx->fn.retro_set_controller_port_device } },

		{ "retro_reset",		{ .retro_reset = &ctx->fn.retro_reset } },
		{ "retro_run",			{ .retro_run = &ctx->fn.retro_run } },
		{ "retro_serialize_size",	{ .retro_serialize_size = &ctx->fn.retro_serialize_size } },
		{ "retro_serialize",		{ .retro_serialize = &ctx->fn.retro_serialize } },
		{ "retro_unserialize",		{ .retro_unserialize = &ctx->fn.retro_unserialize } },

		{ "retro_cheat_reset",		{ .retro_cheat_reset = &ctx->fn.retro_cheat_reset } },
		{ "retro_cheat_set",		{ .retro_cheat_set = &ctx->fn.retro_cheat_set } },
		{ "retro_load_game",		{ .retro_load_game = &ctx->fn.retro_load_game } },
		{ "retro_load_game_special",	{ .retro_load_game_special = &ctx->fn.retro_load_game_special } },
		{ "retro_unload_game",		{ .retro_unload_game = &ctx->fn.retro_unload_game } },
		{ "retro_get_region",		{ .retro_get_region = &ctx->fn.retro_get_region } },

		{ "retro_get_memory_data",	{ .retro_get_memory_data = &ctx->fn.retro_get_memory_data } },
		{ "retro_get_memory_size",	{ .retro_get_memory_size = &ctx->fn.retro_get_memory_size } }
		/* clang-format on */
	};

	ctx->handle = SDL_LoadObject(so_file);
	if(ctx->handle == NULL)
		return 1;

	for(uint_fast8_t i = 0; i < NUM_ELEMS(fn_links); i++)
	{
		*fn_links[i].fn_ptr.sdl_fn =
			SDL_LoadFunction(ctx->handle, fn_links[i].fn_str);

		if(*fn_links[i].fn_ptr.sdl_fn == NULL)
		{
			SDL_UnloadObject(&ctx->handle);
			return 2;
		}
	}

	if(ctx->fn.retro_api_version() != RETRO_API_VERSION)
	{
		SDL_SetError("Incompatible retro API version");
		return 3;
	}

	/* Initialise ctx status information to zero. */
	ctx->env.status = 0;

	return 0;
}

void unload_libretro_core(struct core_ctx_s *ctx)
{
	ctx->fn.retro_deinit();
	free(ctx->game_data);
	ctx->game_data = NULL;
	SDL_UnloadObject(ctx->handle);
}
