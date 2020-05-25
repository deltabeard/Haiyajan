/*
 * tinf - tiny inflate library (inflate, gzip, zlib)
 *
 * Copyright (c) 2003-2019 Joergen Ibsen
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 *   1. The origin of this software must not be misrepresented; you must
 *      not claim that you wrote the original software. If you use this
 *      software in a product, an acknowledgment in the product
 *      documentation would be appreciated but is not required.
 *
 *   2. Altered source versions must be plainly marked as such, and must
 *      not be misrepresented as being the original software.
 *
 *   3. This notice may not be removed or altered from any source
 *      distribution.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/**
 * Status codes returned.
 */
typedef enum {
	TINF_OK         = 0,  /**< Success */
	TINF_DATA_ERROR = 1, /**< Input error */
	TINF_BUF_ERROR  = 2  /**< Not enough room for output */
} tinf_error_code;

/**
 * Decompress `sourceLen` bytes of deflate data from `source` to `dest`.
 *
 * The variable `destLen` points to must contain the size of `dest` on entry,
 * and will be set to the size of the decompressed data on success.
 *
 * Reads at most `sourceLen` bytes from `source`.
 * Writes at most `*destLen` bytes to `dest`.
 *
 * @param dest pointer to where to place decompressed data
 * @param destLen pointer to variable containing size of `dest`
 * @param source pointer to compressed data
 * @param sourceLen size of compressed data
 * @return `TINF_OK` on success, error code on error
 */
tinf_error_code tinf_uncompress(void *dest, size_t *destLen,
		const void *source, size_t sourceLen);

#ifdef __cplusplus
} /* extern "C" */
#endif
