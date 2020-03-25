/**
 * Handles the loading of files, including libretro cores and emulator files.
 */
#include <SDL2/SDL.h>
#include <stdint.h>

#include <libretro.h>
#include <load.h>

#define NUM_ELEMS(x) (sizeof(x) / sizeof(*x))

uint_fast8_t load_libretro_core(const char *so_file, struct core_ctx_s *fn)
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
	} fn_links[] = {
		{ "retro_init", { .retro_init = &fn->retro_init } },
		{ "retro_deinit", { .retro_init = &fn->retro_deinit } },
		{ "retro_api_version", { .retro_api_version = &fn->retro_api_version } },

		{ "retro_set_environment", { .retro_set_environment = &fn->retro_set_environment } },
		{ "retro_set_video_refresh", { .retro_set_video_refresh = &fn->retro_set_video_refresh } },
		{ "retro_set_audio_sample", { .retro_set_audio_sample = &fn->retro_set_audio_sample } },
		{ "retro_set_audio_sample_batch", { .retro_set_audio_sample_batch = &fn->retro_set_audio_sample_batch } },
		{ "retro_set_input_poll", { .retro_set_input_poll = &fn->retro_set_input_poll } },
		{ "retro_set_input_state", { .retro_set_input_state = &fn->retro_set_input_state } },

		{ "retro_get_system_info", { .retro_get_system_info = &fn->retro_get_system_info } },
		{ "retro_get_system_av_info", { .retro_get_system_av_info = &fn->retro_get_system_av_info } },
		{ "retro_set_controller_port_device", { .retro_set_controller_port_device = &fn->retro_set_controller_port_device } },

		{ "retro_reset", { .retro_reset = &fn->retro_reset } },
		{ "retro_run", { .retro_run = &fn->retro_run } },
		{ "retro_serialize_size", { .retro_serialize_size = &fn->retro_serialize_size } },
		{ "retro_serialize", { .retro_serialize = &fn->retro_serialize } },
		{ "retro_unserialize", { .retro_unserialize = &fn->retro_unserialize } },

		{ "retro_cheat_reset", { .retro_cheat_reset = &fn->retro_cheat_reset } },
		{ "retro_cheat_set", { .retro_cheat_set = &fn->retro_cheat_set } },
		{ "retro_load_game", { .retro_load_game = &fn->retro_load_game } },
		{ "retro_load_game_special", { .retro_load_game_special = &fn->retro_load_game_special } },
		{ "retro_unload_game", { .retro_unload_game = &fn->retro_unload_game } },
		{ "retro_get_region", { .retro_get_region = &fn->retro_get_region } },

		{ "retro_get_memory_data", { .retro_get_memory_data = &fn->retro_get_memory_data } },
		{ "retro_get_memory_size", { .retro_get_memory_size = &fn->retro_get_memory_size } }
		/* clang-format on */
	};

	fn->handle = SDL_LoadObject(so_file);
	if(fn->handle == NULL)
		return 1;

	for(uint_fast8_t i = 0; i < NUM_ELEMS(fn_links); i++)
	{
		*fn_links[i].fn_ptr.sdl_fn =
			SDL_LoadFunction(fn->handle, fn_links[i].fn_str);

		if(*fn_links[i].fn_ptr.sdl_fn == NULL)
		{
			SDL_UnloadObject(&fn->handle);
			return 2;
		}
	}

	return 0;
}

void unload_libretro_core(struct core_ctx_s *fn)
{
	SDL_UnloadObject(fn->handle);
}
