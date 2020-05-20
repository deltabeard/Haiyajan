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

#include <SDL2/SDL.h>
#include <x264.h>
#include <wavpack/wavpack.h>

#include <cap.h>

struct enc_vid_s
{
	struct
	{
		SDL_RWops *fa;
		WavpackContext *wpc;
		void *first_block;
		int32_t first_block_sz;
	};

	struct
	{
		SDL_RWops *fv;
		x264_t *h;
		x264_param_t param;
		unsigned frame;

		/* Preset value pointing to x264_preset_names[] */
		Uint8 preset;
		Uint32 delay_limit_ms;
		SDL_SpinLock lock;
		SDL_atomic_t speedmod_timeout;
	};
};

/* Max preset is fast. */
const Uint8 preset_max = 5;

static void x264_log(void *priv, int i_level, const char *fmt, va_list ap)
{
	const SDL_LogPriority lvlmap[] =
	{
		SDL_LOG_PRIORITY_ERROR, SDL_LOG_PRIORITY_WARN,
		SDL_LOG_PRIORITY_INFO, SDL_LOG_PRIORITY_DEBUG
	};
	char buf[256];
	(void)priv;

	/* If log has no level, set to debug. */
	if(i_level < 0)
		i_level = SDL_LOG_PRIORITY_DEBUG;

	SDL_snprintf(buf, SDL_arraysize(buf), "x264: %s", fmt);
	SDL_LogMessageV(SDL_LOG_CATEGORY_VIDEO, lvlmap[i_level], buf, ap);
}

static int wav_pack_write_file(void *priv, void *data, int32_t bcount)
{
	enc_vid *ctx = priv;

	/* If ctx == NULL, then Initialisation error or data is for lossless
	 * wvc data. */
	if(ctx == NULL)
		return SDL_FALSE;

	if(ctx->first_block == NULL)
	{
		/* TODO: Use WavpackUpdateNumSamples() on this before closing file. */
		ctx->first_block = SDL_malloc(bcount);
		SDL_memcpy(ctx->first_block, data, bcount);
		ctx->first_block_sz = bcount;
	}

	return SDL_RWwrite(ctx->fa, data, bcount, 1) > 0 ? SDL_TRUE : SDL_FALSE;
}

enc_vid *vid_enc_init(const char *fileout, int width, int height, double fps,
		      Sint32 sample_rate)
{
	enc_vid *ctx = SDL_calloc(1, sizeof(enc_vid));

	if(ctx == NULL)
		goto out;

	/* Initialise Wavpack */
	const char *wp_ver = WavpackGetLibraryVersionString();
	SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "Initialising Wavpack %s",
		wp_ver);

	/* TODO: Fix this workaround. */
	{
		char *fileout_wav = SDL_strdup(fileout);
		char *dot_loc = SDL_strrchr(fileout_wav, '.');
		*(++dot_loc) = 'w';
		*(++dot_loc) = 'v';
		*(++dot_loc) = '\0';
		ctx->fa = SDL_RWFromFile(fileout_wav, "wb");
		SDL_free(fileout_wav);
	}
	if(ctx->fa == NULL)
		goto err;

	ctx->wpc = WavpackOpenFileOutput(wav_pack_write_file, ctx, NULL);
	WavpackConfig config = {
		.bitrate = 192.0F,
		.bytes_per_sample = 2,
		.bits_per_sample = 16,
		.num_channels = 2,
		.channel_mask = 3,
		.sample_rate = sample_rate,
		.flags = CONFIG_FAST_FLAG | CONFIG_BITRATE_KBPS
	};

	if(WavpackSetConfiguration64(ctx->wpc, &config, -1, NULL) == SDL_FALSE)
	{
		SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO,
			    "Failed to set Wavpack configuration: %s",
			    WavpackGetErrorMessage(ctx->wpc));
		SDL_assert_always(0);
	}

	SDL_assert_always(WavpackPackInit(ctx->wpc));

	x264_param_default(&ctx->param);
	ctx->preset = preset_max;

	/* Get default params for preset/tuning.
	 * Setting preset to veryfast in order to reduce strain during gameplay. */
	if(x264_param_default_preset(&ctx->param,
			x264_preset_names[ctx->preset], "") < 0)
		goto err;

	ctx->param.pf_log = x264_log;
	ctx->param.i_csp = X264_CSP_RGB;
	ctx->param.i_bitdepth = 8;
	ctx->param.b_vfr_input = 0;
	ctx->param.rc.i_rc_method = X264_RC_CRF;
	ctx->param.rc.f_rf_constant = 18;
	ctx->param.rc.f_rf_constant_max = 35;
	ctx->param.b_opencl = 1;

	/* Apply profile restrictions. */
	if(x264_param_apply_profile(&ctx->param, "high444") < 0)
		goto err;

	ctx->param.i_width  = width;
	ctx->param.i_height = height;

	if(fps < 1.0)
		fps = 1.0;

	SDL_assert(fps < 256.0);
	ctx->param.i_fps_num = (uint32_t)(fps * 16777216.0);
	ctx->param.i_fps_den = 16777216;
	SDL_LogVerbose(SDL_LOG_CATEGORY_VIDEO, "Requested video FPS: %.1f",
		((float)ctx->param.i_fps_num/ctx->param.i_fps_den));

	ctx->param.i_threads = 0;
	ctx->param.b_repeat_headers = 0;

	ctx->delay_limit_ms = (1.0/fps) * 1000.0;

	ctx->h = x264_encoder_open(&ctx->param);
	if(ctx->h == NULL)
		goto err;

	ctx->fv = SDL_RWFromFile(fileout, "wb");
	if(ctx->fv == NULL)
		goto err;

	x264_nal_t *nal;
	int nnal;
	if(x264_encoder_headers(ctx->h, &nal, &nnal) < 0)
		goto err;

	for(int i = 0; i < nnal; i++)
	{
		if(SDL_RWwrite(ctx->fv, nal[i].p_payload, nal[i].i_payload, 1) == 0)
			goto err;
	}

out:
	return ctx;

err:
	SDL_free(ctx);
	ctx = NULL;
	goto out;
}

/**
 * All variables required for frame to be encoded and saved.
 */
struct vid_enc_frame_s {
	enc_vid *ctx;
	SDL_Surface *surf;
};

static int vid_enc_frame_thread(void *data)
{
	struct vid_enc_frame_s *framedat = data;
	enc_vid *ctx = framedat->ctx;

	int ret = 1;
	int i_nal;
	int i_frame_size;
	x264_picture_t pic;
	x264_picture_t pic_out;
	x264_nal_t *nal;

	x264_picture_init(&pic);

	pic.img.i_csp = X264_CSP_RGB;
	pic.img.i_plane = 1;
	pic.img.i_stride[0] = framedat->surf->pitch;
	pic.img.plane[0] = framedat->surf->pixels;
#if 0
	pic.i_pts = ctx->frame;
	ctx->frame++;
#endif

	pic.i_type = X264_TYPE_AUTO;

	i_frame_size = x264_encoder_encode(ctx->h, &nal, &i_nal, &pic,
	                                   &pic_out);
	if(i_frame_size < 0)
	{
		SDL_SetError("x264: unable to encode frame");
		goto err;
	}

	if(i_frame_size == 0)
		goto ret;

	for(int i = 0; i < i_nal; i++)
		SDL_RWwrite(ctx->fv, nal[i].p_payload, nal[i].i_payload, 1);

ret:
	ret = 0;
err:
	SDL_AtomicUnlock(&ctx->lock);
	SDL_FreeSurface(framedat->surf);
	SDL_free(data);
	return ret;
}

void vid_enc_speedup(enc_vid *ctx)
{
	if(ctx->preset <= 1)
			return;

	ctx->preset--;
	x264_param_default_preset(&ctx->param, x264_preset_names[ctx->preset],
				"");

	x264_encoder_reconfig(ctx->h, &ctx->param);
	SDL_LogVerbose(SDL_LOG_CATEGORY_VIDEO, "Modified video preset to %s",
				x264_preset_names[ctx->preset]);
}

void vid_enc_speeddown(enc_vid *ctx)
{
	if(ctx->preset == preset_max)
		return;

	ctx->preset++;
	x264_param_default_preset(&ctx->param, x264_preset_names[ctx->preset],
				  "");

	x264_encoder_reconfig(ctx->h, &ctx->param);
	SDL_LogVerbose(SDL_LOG_CATEGORY_VIDEO, "Modified video preset to %s",
				x264_preset_names[ctx->preset]);
}

void vid_enc_frame(enc_vid *ctx, SDL_Surface *surf)
{
	SDL_Thread *thread;
	struct vid_enc_frame_s *th = SDL_malloc(sizeof(struct vid_enc_frame_s));
	th->ctx = ctx;
	th->surf = surf;

	SDL_AtomicLock(&ctx->lock);
	thread = SDL_CreateThread(vid_enc_frame_thread, "x264 Encode", th);
	SDL_DetachThread(thread);
}

void vid_enc_samples(enc_vid *ctx, const Sint16 *data, uint32_t frames)
{
	int ret;
	size_t samples = frames * 2;
	Sint32 *s = SDL_malloc(samples * sizeof(Sint32));

	for(size_t i = 0; i < samples; i++)
	{
		s[i] = data[i];
	}

	ret = WavpackPackSamples(ctx->wpc, s, frames);
	SDL_free(s);
	SDL_assert_always(ret);
}

Sint64 cap_video_size(enc_vid *ctx)
{
	return SDL_RWtell(ctx->fv);
}

Sint64 cap_audio_size(enc_vid *ctx)
{
	return SDL_RWtell(ctx->fa);
}

void vid_enc_end(enc_vid *ctx)
{
	if(ctx == NULL)
		return;

	/* Wait for previous frames to finish encoding. */
	SDL_AtomicLock(&ctx->lock);

	/* Flush delayed frames */
	while(x264_encoder_delayed_frames(ctx->h))
	{
		int i_nal;
		x264_nal_t *nal;
		x264_picture_t pic_out;
		int i_frame_size = x264_encoder_encode(ctx->h, &nal, &i_nal, NULL, &pic_out);
		if(i_frame_size < 0)
			goto out;

		if(i_frame_size == 0)
			continue;

		for(int i = 0; i < i_nal; i++)
			SDL_RWwrite(ctx->fv, nal[i].p_payload, nal[i].i_payload, 1);
	}

	x264_encoder_close(ctx->h);

	WavpackFlushSamples(ctx->wpc);
	WavpackUpdateNumSamples(ctx->wpc, ctx->first_block);
	SDL_RWseek(ctx->fa, 0, RW_SEEK_SET);
	SDL_RWwrite(ctx->fa, ctx->first_block, ctx->first_block_sz, 1);
	WavpackCloseFile(ctx->wpc);

out:
	SDL_RWclose(ctx->fv);
	SDL_RWclose(ctx->fa);
	SDL_AtomicUnlock(&ctx->lock);
	SDL_free(ctx);
	return;
}

