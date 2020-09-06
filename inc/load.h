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

struct fn_links_s
{
	/* Name of libretro core function. */
	const char *fn_str;
	void **fn_ptr;
};

struct links_list_s
{
	const void **fn_retro;
	const void **fn_re;
};

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

/**
 * Searches built-in libretro cores for their compatibility with the given
 * file extension.
 *
 * \param ext	File extension.
 * \param ptr	Opaque pointer. Must be NULL for first call of search.
 * \returns	Library name or NULL. Repeat call until NULL is returned.
 */
const char *load_get_supported_internal_cores(const char *ext, void **ptr);

SDL_bool load_internal_libretro_core(const char *corename, struct core_ctx_s *ctx);
