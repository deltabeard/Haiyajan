/**
 * Converts a libretro.c file for a libretro core to a static library.
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

#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static const char *libretro_fns[] = {
	"retro_init",
	"retro_deinit",
	"retro_api_version",
	"retro_set_environment",
	"retro_set_video_refresh",
	"retro_set_audio_sample",
	"retro_set_audio_sample_batch",
	"retro_set_input_poll",
	"retro_set_input_state",
	"retro_get_system_info",
	"retro_get_system_av_info",
	"retro_set_controller_port_device",
	"retro_reset",
	"retro_run",
	"retro_serialize_size",
	"retro_serialize",
	"retro_unserialize",
	"retro_cheat_reset",
	"retro_cheat_set",
	"retro_load_game",
	"retro_load_game_special",
	"retro_unload_game",
	"retro_get_region",
	"retro_get_memory_data",
	"retro_get_memory_size",
	"re_core_get_license_info",
	"re_core_set_pause"
};

int main(int argc, char *argv[])
{
	char *line = NULL;
	size_t sz = 0;
	unsigned fns = sizeof(libretro_fns)/sizeof(*libretro_fns);
	char *suffix;

	if(argc != 2)
	{
		fputs("Usage: libretro2static suffix\n"
			"  Pipe libretro.c to stdin. Output via stdout.\n"
			"\0", stderr);
		return EXIT_FAILURE;
	}

	suffix = argv[1];

	while(getline(&line, &sz, stdin) != -1)
	{
		char *found;
		size_t prefix_len = 0;

		for(unsigned i = 0; i < fns; i++)
		{
			char *end;
			size_t foundlen;

			if((found = strstr(line, libretro_fns[i])) == NULL)
				continue;

			end = strchr(found, '(');
			foundlen = end - found;
			if(foundlen != strlen(libretro_fns[i]))
				continue;

			prefix_len = strlen(libretro_fns[i]) + found - line;
			break;
		}

		if(found != NULL)
		{
			fprintf(stdout, "%.*s_", (int)prefix_len, line);
			fputs(suffix, stdout);
		}

		fputs(line + prefix_len, stdout);
	}

	return EXIT_SUCCESS;
}

#if 0
struct links_list_s
{
	void **fn_retro;
	void **fn_re;
};
const void *fn_links_snes9x2010[] =
{
	(void *)retro_init_snes9x2010,
	(void *)retro_deinit_snes9x2010,
	(void *)retro_api_version_snes9x2010,
	(void *)retro_set_environment_snes9x2010,
	(void *)retro_set_video_refresh_snes9x2010,
	(void *)retro_set_audio_sample_snes9x2010,
	(void *)retro_set_audio_sample_batch_snes9x2010,
	(void *)retro_set_input_poll_snes9x2010,
	(void *)retro_set_input_state_snes9x2010,
	(void *)retro_get_system_info_snes9x2010,
	(void *)retro_get_system_av_info_snes9x2010,
	(void *)retro_set_controller_port_device_snes9x2010,
	(void *)retro_reset_snes9x2010,
	(void *)retro_run_snes9x2010,
	(void *)retro_serialize_size_snes9x2010,
	(void *)retro_serialize_snes9x2010,
	(void *)retro_unserialize_snes9x2010,
	(void *)retro_cheat_reset_snes9x2010,
	(void *)retro_cheat_set_snes9x2010,
	(void *)retro_load_game_snes9x2010,
	(void *)retro_load_game_special_snes9x2010,
	(void *)retro_unload_game_snes9x2010,
	(void *)retro_get_region_snes9x2010,
	(void *)retro_get_memory_data_snes9x2010,
	(void *)retro_get_memory_size_snes9x2010
};
const void *ext_fn_links_snes9x2010[] = {
	(void *)re_core_get_license_info_snes9x2010,
	(void *)re_core_set_pause_snes9x2010
};

const struct links_list_s *const internal_cores[] = {
	{ &fn_links_snes9x2010, &ext_fn_links_snes9x2010 },
	{ NULL, NULL }
}
#endif
