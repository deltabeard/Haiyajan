/**
 * Record video with vp9 encoder.
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

#include <SDL2/SDL.h>
#include <vpx/vpx_encoder.h>

struct vp9_enc_s {
	SDL_RWops *out;
};
typedef struct vp9_enc_s vp9_enc;

/**
 * \param outfile	Name of the output file to write to.
 * \param w		Width of display.
 * \param h		Height of display.
 * \param fps		Frame rate.
 * \return		Encoder context or NULL on error.
 */
vp9_enc *vp9_start(const char *outfile, int w, int h, double fps)
{
	vp9_enc *ctx = SDL_calloc(1, sizeof(vp9_enc));

	// Create file
	// TODO: Later on generate file header.
	// Initialise VP9 encoder.
}

int vp9_write_frame(vp9_enc *ctx, SDL_Surface *surf)
{
	// Convert surface to IYUV
	return 0;
}

int vp9_end(vp9_enc *ctx)
{
	return 0;
}
