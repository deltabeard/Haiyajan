/**
 * Various utility functions for Haiyajan.
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

#ifdef _WIN32
#define atleast
#else
#define atleast static
#endif

/**
 * Generates a filename based upon the current time, and the core name. Filename
 * will be a maximum of 63 characters with a NULL terminator.
 *
 * \param filename	String array of at least 64 bytes.
 * \param core_name	Name of the core. Can be NULL if not available.
 * \param fmt		File format to use in filename.
 */
void gen_filename(char filename[atleast 64], const char *core_name,
		  const char fmt[atleast 3]);

/**
 * Modify an atomic variable to a value after a given time.
 *
 * \param timeout_ms	Time to wait in ms before modifying atomic variable.
 * \param atomic	The atomic variable to modify.
 * \param setval	The value to set the atomic variable to after timeout.
 * \param name		The name of the thread. Can be set to NULL.
 */
void set_atomic_timeout(Uint32 timeout_ms, SDL_atomic_t *atomic, int setval,
	const char *name);

/**
 * Makes sure that delayed threads no longer attempt to modify data that may
 * now be out of scope.
 */
void util_exit_all(void);

/**
 * Converts SDL_Texture to SDL_Surface by drawing the given texture onto the
 * renderer, and reading the pixels back into a surface.
 *
 * \param rend		Renderer associated with the given texture.
 * \param tex		Texture to convert to surface.
 * \param src		Area of Texture to convert to surface. Passing NULL will
 * 			convert total area.
 * \param flip		Whether to flip the texture before conversion.
 * \return		SDL_Surface or NULL on error.
 */
SDL_Surface *util_tex_to_surf(SDL_Renderer *rend, SDL_Texture *tex,
			      const SDL_Rect *const src,
			      const SDL_RendererFlip flip);

/**
 * Copies bytes from src to dest, stopping after the first occurance of byte c
 * is copied, or after n bytes are copied. Source and destination must not
 * overlap.
 * \param dest	Destination to copy bytes to.
 * \param src	Source to copy bytes from.
 * \param c	Character to check on each byte copy and to terminate on.
 * \param n	Maximum number of bytes to copy.
 * \return	Null if c was not found, or pointer to byte after the occurance
 * 		of c at the destination.
 */
void *util_memccpy(void *restrict dest, const void *restrict src, int c,
		size_t n);
