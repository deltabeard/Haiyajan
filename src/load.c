/**
 * Handles the loading of files, including libretro cores and emulator files.
 */
#include <SDL2/SDL.h>
#include <stdint.h>

#include <libretro.h>
#include <load.h>

uint_fast8_t initialise_libretro_core(const char *so_file,
		struct libretro_fn_s *fn)
{
	(void)so_file;
	(void)fn;
	return 0;
}
