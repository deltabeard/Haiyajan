#pragma once

#include <libretro.h>

struct libretro_fn_s {
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
};

/**
 * Initialises a libretro core.
 *
 * \param so_file	File path to libretro object.
 * \param fn		Struct of libretro function pointers to initialise.
 * \return		0 on error, else success.
 */
uint_fast8_t initialise_libretro_core(const char *so_file,
		struct libretro_fn_s *fn);
