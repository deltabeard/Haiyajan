#pragma once

#include <parsley.h>

/**
 * Returns the retro API version that load.c was compiled with.
 *
 * \return 1 always.
 */
unsigned load_compiled_retro_api_version(void);

/**
 * Loads a libretro core and assigns its functions to the given libretro
 * function pointer struct.
 *
 * \param so_file	File path to libretro object.
 * \param fn		Struct of libretro function pointers to initialise.
 * \return		0 on success, else failure. Use SDL_GetError().
 */
uint_fast8_t load_libretro_core(const char *so_file, struct core_ctx_s *fn);

/**
 * Unloads a libretro core.
 * Calling any libretro function inside the given context after calling this
 * function is undefined.
 *
 * \param fn Struct of initialised libretro function pointers.
 */
void unload_libretro_core(struct core_ctx_s *fn);
