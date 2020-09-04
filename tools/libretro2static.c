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
			if((found = strstr(line, libretro_fns[i])) == NULL)
				continue;


			prefix_len = strlen(libretro_fns[i]) + found - line;
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
