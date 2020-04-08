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

#pragma once

#include <libretro.h>

#define PROG_NAME     "Parsley"
#define PROG_NAME_LEN strlen(PROG_NAME)
#define MAX_TITLE_LEN 56

/**
 * Context of libretro core.
 */
struct core_ctx_s
{
	/* Libretro core functions. */
	struct
	{
		/* clang-format off */
		void (*retro_init)(void);
		void (*retro_deinit)(void);
		unsigned (*retro_api_version)(void);

		void (*retro_set_environment)(retro_environment_t);
		void (*retro_set_video_refresh)(retro_video_refresh_t);
		void (*retro_set_audio_sample)(retro_audio_sample_t);
		void (*retro_set_audio_sample_batch)(retro_audio_sample_batch_t);
		void (*retro_set_input_poll)(retro_input_poll_t);
		void (*retro_set_input_state)(retro_input_state_t);

		void (*retro_get_system_info)(struct retro_system_info *info);
		void (*retro_get_system_av_info)(struct retro_system_av_info *info);
		void (*retro_set_controller_port_device)(unsigned port, unsigned device);

		void (*retro_reset)(void);
		void (*retro_run)(void);
		size_t (*retro_serialize_size)(void);
		bool (*retro_serialize)(void *data, size_t size);
		bool (*retro_unserialize)(const void *data, size_t size);

		void (*retro_cheat_reset)(void);
		void (*retro_cheat_set)(unsigned index, bool enabled, const char *code);
		bool (*retro_load_game)(const struct retro_game_info *game);
		bool (*retro_load_game_special)(unsigned game_type, const struct retro_game_info *info, size_t num_info);
		void (*retro_unload_game)(void);
		unsigned (*retro_get_region)(void);

		void *(*retro_get_memory_data)(unsigned id);
		size_t (*retro_get_memory_size)(unsigned id);
		/* clang-format on */
	} fn;

	/* SDL2 handles. */
	struct
	{
		/* SDL2 function object handle for libretro core. */
		void *handle;

		/* For cores which require the game to be loaded into memory. */
		Uint8 *game_data;

		SDL_Renderer *disp_rend;
		SDL_Texture *game_texture;
		SDL_Rect game_logical_res;
	};

	/* Libretro core information. */
	struct retro_system_info sys_info;
	struct retro_system_av_info av_info;

	/* Libretro core environment status. */
	struct
	{
		union {
			struct
			{
				unsigned char core_init : 1;
				unsigned char shutdown : 1;
				unsigned char game_loaded : 1;
				unsigned char running : 1;
			} status_bits;
			Uint8 status;
		};

		unsigned perf_lvl;
		Uint32 pixel_fmt;
	} env;
};
