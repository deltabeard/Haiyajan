/**
 * Handles the loading of files, including libretro cores and emulator files.
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

#include <haiyajan.h>
#include <libretro.h>
#include <load.h>

static void save_sram_file(struct core_ctx_s *ctx)
{
	SDL_RWops *sram_rw;
	size_t sram_size;
	void *sram_dat;

	if(ctx->sram_filename == NULL)
		goto out;

	sram_rw = SDL_RWFromFile(ctx->sram_filename, "wb");
	if(sram_rw == NULL)
		goto out;

	sram_size = ctx->fn.retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);
	sram_dat = ctx->fn.retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);
	if(sram_dat != NULL)
		SDL_RWwrite(sram_rw, sram_dat, sram_size, 1);

	SDL_RWclose(sram_rw);
	SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "Saved SRAM file.");

out:
	SDL_free(ctx->sram_filename);
	ctx->sram_filename = NULL;

	return;
}

static void load_sram_file(struct core_ctx_s *ctx)
{
	SDL_RWops *sram_rw;
	size_t sram_size;
	size_t sram_size_exp;
	size_t sram_len;
	void *sram_dat;

	if(ctx->content_filename == NULL)
		goto out;

	ctx->sram_filename = SDL_strdup(ctx->content_filename);
	if(ctx->sram_filename == NULL)
		goto out;

	sram_len = SDL_strlen(ctx->sram_filename) - 1;
	*(ctx->sram_filename + sram_len--) = 'm';
	*(ctx->sram_filename + sram_len--) = 'r';
	*(ctx->sram_filename + sram_len--) = 's';
	*(ctx->sram_filename + sram_len--) = '.';

	sram_rw = SDL_RWFromFile(ctx->sram_filename, "rb");
	if(sram_rw == NULL)
	{
		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
			    "Save file not found; a new one will be created.");
		goto out;
	}

	sram_size = SDL_RWsize(sram_rw);
	sram_size_exp =
		ctx->fn.retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);
	sram_size = sram_size > sram_size_exp ? sram_size_exp : sram_size;
	sram_dat = ctx->fn.retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);
	if(sram_dat != NULL)
		SDL_RWread(sram_rw, sram_dat, sram_size, 1);

	SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "Read SRAM file.");
	SDL_RWclose(sram_rw);

out:
	return;
}

int load_libretro_file(struct core_ctx_s *ctx)
{
	/* TODO:
	 * - Check whether file must be loaded into RAM, or is read straight
	 * from disk.
	 * - Read file.
	 *
	 * If need_fullpath in retro_system_info is true, then don't load the
	 * ROM into memory. If false, then Haiyajan must load the ROM image into
	 * memory.
	 */
	struct retro_game_info game;
	struct retro_game_info *gamep = &game;

	game.path = ctx->content_filename;
	game.meta = NULL;

	SDL_assert_paranoid(ctx != NULL);
	SDL_assert(ctx->env.status.bits.core_init == 1);

	if(ctx->content_filename == NULL &&
			ctx->env.status.bits.support_no_game == SDL_FALSE)
	{
		SDL_SetError("This libretro core requires a content file.");
		return 1;
	}
	else if(ctx->content_filename == NULL &&
			ctx->env.status.bits.support_no_game == SDL_TRUE)
	{
		gamep = NULL;
	}
	else if(ctx->sys_info.need_fullpath == true)
	{
		ctx->sdl.game_data = NULL;
		game.data = NULL;
	}
	else
	{
		ctx->sdl.game_data = SDL_LoadFile(ctx->content_filename, &game.size);

		if(ctx->sdl.game_data == NULL)
			return 1;

		game.data = ctx->sdl.game_data;
	}

	if(ctx->fn.retro_load_game(gamep) == false)
	{
		SDL_SetError("Libretro core failed to load content");
		return 1;
	}

	load_sram_file(ctx);
	ctx->env.status.bits.game_loaded = 1;
	ctx->env.status.bits.shutdown = 0;

	return 0;
}

#if 0
/* TODO: Function for later consideration. */
int load_is_libretro_core(const char *file)
{
	SDL_RWops *f;
	Uint8 header[4];
	const Uint8 elf[4] = { 0x7F, 'E', 'L', 'F' };
	const Uint8 dll[2] = { 'M', 'Z' };
	const Uint8 dylib[4] = { 0xCF, 0xFA, 0xED, 0xFE };

	f = SDL_RWFromFile(file, "rb");
	if(f == NULL)
		return -1;

	if(SDL_RWread(f, header, 1, sizeof(header)) != sizeof(header))
	{
		SDL_RWclose(f);
		return -1;
	}

	SDL_RWclose(f);

	/* Check if an ELF executable. */
	if(SDL_memcmp(header, elf, sizeof(elf)) == 0)
		return 1;

	/* Check if an DLL executable. */
	if(SDL_memcmp(header, dll, sizeof(dll)) == 0)
		return 1;

	/* Check if an DYLIB executable. */
	if(SDL_memcmp(header, dylib, sizeof(dylib)) == 0)
		return 1;

	return 0;
}
#endif

static SDL_bool does_core_support_ext(const char *ext, const char *valid_ext)
{
	SDL_bool ret = SDL_FALSE;
	char *token;
	char *saveptr;
	char *str;

	str = SDL_strdup(valid_ext);
	if(str == NULL)
		goto out;

	token = SDL_strtokr(str, "|", &saveptr);
	if(token == NULL)
		goto out;

	do
	{
		if(SDL_strcasecmp(token, ext) != 0)
			continue;

		ret = SDL_TRUE;
		break;
	} while((token = SDL_strtokr(NULL, "|", &saveptr)) != NULL);

out:
	SDL_free(str);
	return ret;
}

/**
 * A pointer to an external array of string pointers that store the name of
 * libretro cores that are statically linked with Haiyajan.
 * Array must be NULL terminated.
 */
extern const struct links_list_s internal_cores[];

const char *load_get_supported_internal_cores(const char *ext, void **ptr)
{
	const struct links_list_s **ic;
	const struct links_list_s *retro;

	SDL_assert(ptr != NULL);

	ic = ptr;
	if(*ic == NULL)
		*ic = internal_cores;
	else
	{
		(*ic)++;
		if((*ic)->fn_retro == NULL)
			goto out;
	}

	for(retro = *ic; retro->fn_retro != NULL; retro++)
	{
		const unsigned rgsi_index = 9;
		struct retro_system_info info;
		void (*retro_get_system_info)(struct retro_system_info *);

		retro_get_system_info = retro->fn_retro[rgsi_index];
		retro_get_system_info(&info);
		if(does_core_support_ext(ext, info.valid_extensions) == SDL_FALSE)
			continue;

		return info.library_name;
	}

out:
	return NULL;
}

SDL_bool load_internal_libretro_core(const char *corename, struct core_ctx_s *ctx)
{
	const struct links_list_s *retro;

	for(retro = internal_cores; retro->fn_retro != NULL; retro++)
	{
		const unsigned rgsi_index = 9;
		struct retro_system_info info;
		void (*retro_get_system_info)(struct retro_system_info *);

		retro_get_system_info = retro->fn_retro[rgsi_index];
		retro_get_system_info(&info);

		if(SDL_strcasecmp(corename, info.library_name) != 0)
			continue;

		SDL_memcpy(&ctx->fn, retro->fn_retro, sizeof(ctx->fn));
		/* Not doing an API check. If this is statically linked, then
		 * libretro.h should be on the same version at compile time? */
		return SDL_TRUE;
	}

	return SDL_FALSE;
}

int load_libretro_core(const char *so_file, struct core_ctx_s *ctx)
{
	unsigned i;
	const struct fn_links_s fn_links[] =
	{
		{ "retro_init",			(void **)&ctx->fn.retro_init },
		{ "retro_deinit",		(void **)&ctx->fn.retro_deinit },
		{ "retro_api_version",		(void **)&ctx->fn.retro_api_version },

		{ "retro_set_environment",	(void **)&ctx->fn.retro_set_environment },
		{ "retro_set_video_refresh",	(void **)&ctx->fn.retro_set_video_refresh },
		{ "retro_set_audio_sample",	(void **)&ctx->fn.retro_set_audio_sample },
		{ "retro_set_audio_sample_batch", (void **)&ctx->fn.retro_set_audio_sample_batch },
		{ "retro_set_input_poll",	(void **)&ctx->fn.retro_set_input_poll },
		{ "retro_set_input_state",	(void **)&ctx->fn.retro_set_input_state },

		{ "retro_get_system_info",	(void **)&ctx->fn.retro_get_system_info },
		{ "retro_get_system_av_info",	(void **)&ctx->fn.retro_get_system_av_info },
		{ "retro_set_controller_port_device", (void **)&ctx->fn.retro_set_controller_port_device },

		{ "retro_reset",		(void **)&ctx->fn.retro_reset },
		{ "retro_run",			(void **)&ctx->fn.retro_run },
		{ "retro_serialize_size",	(void **)&ctx->fn.retro_serialize_size },
		{ "retro_serialize",		(void **)&ctx->fn.retro_serialize },
		{ "retro_unserialize",		(void **)&ctx->fn.retro_unserialize },

		{ "retro_cheat_reset",		(void **)&ctx->fn.retro_cheat_reset },
		{ "retro_cheat_set",		(void **)&ctx->fn.retro_cheat_set },
		{ "retro_load_game",		(void **)&ctx->fn.retro_load_game },
		{ "retro_load_game_special",	(void **)&ctx->fn.retro_load_game_special },
		{ "retro_unload_game",		(void **)&ctx->fn.retro_unload_game },
		{ "retro_get_region",		(void **)&ctx->fn.retro_get_region },

		{ "retro_get_memory_data",	(void **)&ctx->fn.retro_get_memory_data },
		{ "retro_get_memory_size",	(void **)&ctx->fn.retro_get_memory_size }
		/* clang-format on */
	};
	const struct fn_links_s ext_fn_links[] = {
		{ "re_core_get_license_info",	(void **)&ctx->ext_fn.re_core_get_license_info},
		{ "re_core_set_pause",		(void **)&ctx->ext_fn.re_core_set_pause}
	};

	ctx->sdl.handle = SDL_LoadObject(so_file);

	if(ctx->sdl.handle == NULL)
		return 1;

	for(i = 0; i < SDL_arraysize(fn_links); i++)
	{
		*fn_links[i].fn_ptr =
			SDL_LoadFunction(ctx->sdl.handle, fn_links[i].fn_str);

		if(*fn_links[i].fn_ptr== NULL)
		{
			if(ctx->sdl.handle != NULL)
				SDL_UnloadObject(&ctx->sdl.handle);

			ctx->sdl.handle = NULL;
			return 2;
		}
	}

	for(i = 0; i < SDL_arraysize(ext_fn_links); i++)
	{
		*ext_fn_links[i].fn_ptr =
			SDL_LoadFunction(ctx->sdl.handle, ext_fn_links[i].fn_str);
	}

	if(ctx->fn.retro_api_version() != RETRO_API_VERSION)
	{
		SDL_SetError("Incompatible retro API version");
		return 3;
	}

	return 0;
}

void unload_libretro_file(struct core_ctx_s *ctx)
{
	save_sram_file(ctx);

	if(ctx->sdl.game_data != NULL)
	{
		SDL_free(ctx->sdl.game_data);
		ctx->sdl.game_data = NULL;
	}

	ctx->env.status.bits.game_loaded = 0;
}

void unload_libretro_core(struct core_ctx_s *ctx)
{
	if(ctx->env.status.bits.game_loaded)
		unload_libretro_file(ctx);

	if(ctx->fn.retro_deinit != NULL)
		ctx->fn.retro_deinit();

	ctx->env.status.bits.core_init = 0;

	if(ctx->sdl.handle != NULL)
	{
		SDL_UnloadObject(ctx->sdl.handle);
		ctx->sdl.handle = NULL;
	}

	SDL_zero(ctx->fn);
}
