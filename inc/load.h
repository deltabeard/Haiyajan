#pragma once

#include <parsley.h>

/**
 * Loads a libretro core and assigns its functions to the given libretro
 * function pointer struct.
 *
 * \param so_file	File path to libretro object.
 * \param fn		Struct of libretro function pointers to initialise.
 * \return		0 on success, else failure.
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
