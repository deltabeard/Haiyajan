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

#pragma once

#include <SDL2/SDL.h>

#include <libretro.h>
#include <input.h>
#include <gl.h>

struct settings_s
{
	unsigned char vid_info : 1;
	unsigned char fullscreen : 1;
	unsigned char benchmark : 1;
};

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
		bool (*retro_load_game_special)(unsigned game_type,
			const struct retro_game_info *info, size_t num_info);
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

		/* For cores which require the content to be loaded into memory.
		 * This is an allocated buffer of content for libretro cores. */
		Uint8 *game_data;

		SDL_Window *win;

		/* The renderer that is shown on screen. */
		SDL_Renderer *disp_rend;

		/* The texture that the libretro core renders to. */
		/* FIXME: Make texture max-width/height */
		SDL_Texture *core_tex;

		/* The resolution of the libretro core video output. */
		/* FIXME: Use video_cb width/height for window size. */
		SDL_Rect game_logical_res;
		SDL_Rect game_target_res;

		/* The context of the audio device. */
		SDL_AudioDeviceID audio_dev;
	};

	glctx *gl;

	/* Libretro core information. */
	struct retro_system_info sys_info;
	struct retro_system_av_info av_info;

	struct
	{
		char core_log_name[10];
		char *file_core;
		char *file_content;
		char *file_content_sram;
		char path_sep;
	};

	/* Libretro core environment status. */
	struct
	{
		union
		{
			struct
			{
				unsigned char core_init : 1;
				unsigned char shutdown : 1;
				unsigned char game_loaded : 1;
				unsigned char running : 1;
				unsigned char video_disabled : 1;
			} status_bits;
			Uint8 status;
		};

		unsigned perf_lvl;
		Uint32 pixel_fmt;
		struct retro_audio_callback audio_cb;
		retro_frame_time_callback_t ftcb;
		retro_usec_t ftref;
	} env;

	struct input_ctx_s inp;
	struct settings_s stngs;
};
