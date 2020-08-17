/**
 * Handles the loading of files, including libretro cores and emulator files.
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

#include <haiyajan.h>

/**
 * Loads a file for the libretro core.
 *
 * \param file	File path of file.
 * \param ctx	Libretro core context.
 * \return	0 on success, else failure. Use SDL_GetError().
 */
int load_libretro_file(struct core_ctx_s *ctx);

/**
 * Loads a libretro core and assigns its functions to the given libretro
 * function pointer struct.
 *
 * \param so_file	File path to libretro object.
 * \param ctx		Libretro core context to initialise.
 * \return		0 on success, else failure. Use SDL_GetError().
 */
int load_libretro_core(const char *so_file, struct core_ctx_s *ctx);

/**
 * Unloads any file that was opened for the libretro core.
 *
 * \param ctx	Libretro core context.
 */
void unload_libretro_file(struct core_ctx_s *ctx);

/**
 * Unloads a libretro core.
 * Calling any libretro function inside the given context after calling this
 * function is undefined.
 *
 * \param ctx Libretro core context to unload.
 */
void unload_libretro_core(struct core_ctx_s *ctx);
