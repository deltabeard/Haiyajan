/**
 * Capture video and audio with x264 and Wavpack encoders.
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

typedef struct rec_s rec;

rec *rec_init(const char *fileout, int width, int height, double fps,
		      Sint32 sample_rate);
void rec_enc_video(rec *ctx, SDL_Surface *surf);
void rec_enc_audio(rec *ctx, const Sint16 *data, uint32_t frames);
void rec_end(rec **ctxp);

Sint64 rec_video_size(rec *ctx);
Sint64 rec_audio_size(rec *ctx);
void rec_set_crf(rec *ctx, Uint8 crf);
void rec_speedup(rec *ctx);
void rec_relax(rec *ctx);

