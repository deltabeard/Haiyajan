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

#if ENABLE_VIDEO_RECORDING == 1
typedef struct rec_s rec;

/**
 * Initialise video recording context.
 * Video is in H264 RGB24 4:4:4 format. Not many decoders support this format.
 * Any decoder that uses a recent version of libx264 should be able to decode
 * this format. This includes the latest ffmpeg.
 * Audio is encoded with Wavpack. This is primarily due to supporting any input
 * sample rate.
 * These files are not merged into a container format.
 *
 * Software encoding is used for both audio and video. This will consume
 * significant CPU time.
 *
 * TODO: Use ffmpeg hardware encoding.
 *
 * \param fileout	Output file name.
 * \param width		Width of video.
 * \param height	Height of video.
 * \param fps		Frames per second.
 * \param sample_rate	Sample rate of audio.
 * \return		Valid context used for recording, or NULL on error.
 */
rec *rec_init(const char *fileout, int width, int height, double fps,
		      Sint32 sample_rate);

/**
 * Encode given surface as a new frame of video.
 * This may take too long too complete. Use rec_speedup() and rec_relax() to
 * control the time taken to encode frames.
 */
void rec_enc_video(rec *ctx, SDL_Surface *surf);

/**
 * Encode a given number of audio frames.
 */
void rec_enc_audio(rec *ctx, const Sint16 *data, uint32_t frames);

/**
 * Finish encoding video and audio, and save to output file.
 * The recording context is free'd and invalidated after this call.
 */
void rec_end(rec **ctxp);

/**
 * Returns the current output file size of the video file, or -1 on error.
 */
Sint64 rec_video_size(rec *ctx);

/**
 * Returns the current output file size of the audio file, or -1 on error.
 */
Sint64 rec_audio_size(rec *ctx);

/**
 * Set the quality of the video.
 */
void rec_set_crf(rec *ctx, Uint8 crf);

/**
 * Improve the speed of video encoding by reducing the quality of the output.
 */
void rec_speedup(rec *ctx);

/**
 * Improve the quality of encoded video by increasing the time spent processing
 * the input frames.
 */
void rec_relax(rec *ctx);
#endif

/**
 * Save a surface to a file.
 */
void rec_single_img(SDL_Surface *surf, const char *core_name);
