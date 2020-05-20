/**
 * Capture video with x264 encoder.
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

typedef struct enc_vid_s enc_vid;

enc_vid *vid_enc_init(const char *fileout, int width, int height, double fps,
		      Sint32 sample_rate);
void vid_enc_frame(enc_vid *ctx, SDL_Surface *surf);
void vid_enc_samples(enc_vid *ctx, const void *samples, uint32_t frames);
Sint64 cap_video_size(enc_vid *ctx);
Sint64 cap_audio_size(enc_vid *ctx);
void vid_enc_speedup(enc_vid *ctx);
void vid_enc_speeddown(enc_vid *ctx);
void vid_enc_end(enc_vid *ctx);
