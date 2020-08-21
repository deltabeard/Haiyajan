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

#include <SDL.h>

#include <font.h>
#include <gl.h>
#include <input.h>
#include <libretro.h>
#include <retro-extensions.h>
#include <rec.h>
#include <timer.h>
#include <ui.h>

#define REL_VERSION_MAJOR 0
#define REL_VERSION_MINOR 0

struct settings_s
{
	unsigned vid_info : 1;
	unsigned fullscreen : 1;
	unsigned benchmark : 1;
	unsigned start_core : 1;
	Uint32 benchmark_dur;
	Uint8 frameskip_limit;
	char *core_filename;
	char *content_filename;
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

	/* Retro extension functions are not mandatory. When not used,
	 * they are set to NULL. */
	struct {
		const struct license_info_s *(*re_core_get_license_info)(void);
		void (*re_core_set_pause)(int pause);
	} ext_fn;

	/* SDL2 handles. */
	struct
	{
		/* SDL2 function object handle for libretro core. */
		void *handle;

		/* For cores which require the content to be loaded into memory.
		 * This is an allocated buffer of content for libretro cores. */
		Uint8 *game_data;

		/* The texture that the libretro core renders to. */
		SDL_Texture *core_tex;

		/* The maximum resolution of the libretro core video output.
		 * The texture must be at least this size. x and y must be 0. */
		SDL_Rect game_max_res;

		/* The resolution of the drawn frame. x and y must be 0. */
		SDL_Rect game_frame_res;

		/* The context of the audio device. */
		SDL_AudioDeviceID audio_dev;

		/* OpenGL context for Libretro Cores. */
		gl_ctx *gl;
	} sdl;

	/* Libretro core information. */
	struct retro_system_info sys_info;
	struct retro_system_av_info av_info;

	char core_short_name[10];
	char *core_filename;
	char *content_filename;
	char *sram_filename;

	/* Libretro core environment status. */
	struct
	{
		union
		{
			struct
			{
				unsigned core_init : 1;
				unsigned shutdown : 1;
				unsigned game_loaded : 1;
				unsigned av_init : 1;
				unsigned opengl_required : 1;
				unsigned playing : 1;
				unsigned video_disabled : 1;
				unsigned valid_frame : 1;
			} bits;
			Uint8 all;
		} status;

		unsigned perf_lvl;
		Uint32 pixel_fmt;
		Uint32 frames;
		SDL_RendererFlip flip;

		struct retro_audio_callback audio_cb;
		retro_frame_time_callback_t ftcb;
		retro_usec_t ftref;
	} env;

	struct timer_ctx_s tim;
	struct input_ctx_s inp;

#if ENABLE_VIDEO_RECORDING == 1
	rec_ctx *vid;
#endif
};

struct haiyajan_ctx_s
{
	/* The SDL2 Window context. */
	SDL_Window *win;

	/* The renderer that is shown on screen. */
	SDL_Renderer *rend;

	/* Haiyajan Settings. */
	struct settings_s stngs;

	/* Core texture target dimensions. */
	SDL_Rect core_tex_targ;

	/* Libretro core context. */
	struct core_ctx_s core;

	/* User interface context. */
	ui *ui;
	ui_overlay_ctx *ui_overlay;

	/* Font */
	font_ctx *font;

	unsigned quit : 1;
};

