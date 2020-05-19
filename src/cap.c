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

#include <cap.h>

struct enc_vid_s
{
	SDL_RWops *f;
	x264_t *h;
	x264_param_t param;
	unsigned frame;

	/* Preset value pointing to x264_preset_names[] */
	Uint8 preset;
	Uint8 max_delay_ms;
	Uint8 min_delay_ms;
	Uint32 delay_ms;
	SDL_SpinLock lock;
	SDL_atomic_t speedmod_timeout;
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

enc_vid *vid_enc_init(const char *fileout, int width, int height, double fps)
{
	enc_vid *ctx = SDL_calloc(1, sizeof(enc_vid));

	if(ctx == NULL)
		goto out;

	x264_param_default(&ctx->param);
	ctx->preset = preset_max;

	/* Get default params for preset/tuning.
	 * Setting preset to veryfast in order to reduce strain during gameplay. */
	if(x264_param_default_preset(&ctx->param,
			x264_preset_names[ctx->preset], "zerolatency") < 0)
		goto err;

	ctx->param.pf_log = x264_log;
	ctx->param.i_csp = X264_CSP_RGB;
	ctx->param.i_bitdepth = 8;
	ctx->param.b_vfr_input = 0;

	ctx->param.rc.i_rc_method = X264_RC_CRF;
	ctx->param.rc.f_rf_constant = 18;
	//ctx->param.rc.f_rf_constant_max = 35;
	ctx->param.b_opencl = 1;

	/* Apply profile restrictions. */
	if(x264_param_apply_profile(&ctx->param, "high444") < 0)
		goto err;

	ctx->param.i_width  = width;
	ctx->param.i_height = height;

	SDL_assert(fps < 256.0);
	ctx->param.i_fps_num = (uint32_t)(fps * 16777216.0);
	ctx->param.i_fps_den = 16777216;

	ctx->param.i_threads = SDL_GetCPUCount();
	ctx->param.b_repeat_headers = 0;

	ctx->min_delay_ms = SDL_ceil((1.0/fps) * 1000.0 * 0.4);
	ctx->max_delay_ms = SDL_floor((1.0/fps) * 1000.0);
	ctx->delay_ms = SDL_GetTicks();

	ctx->h = x264_encoder_open(&ctx->param);
	if(ctx->h == NULL)
		goto err;

	ctx->f = SDL_RWFromFile(fileout, "wb");
	if(ctx->f == NULL)
		goto err;

	x264_nal_t *nal;
	int nnal;
	if(x264_encoder_headers(ctx->h, &nal, &nnal) < 0)
		goto err;

	for(int i = 0; i < nnal; i++)
	{
		if(SDL_RWwrite(ctx->f, nal[i].p_payload, nal[i].i_payload, 1) == 0)
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

#if 0
	Uint32 now_ms = SDL_GetTicks();
	if((now_ms - ctx->delay_ms) > ctx->max_delay_ms && ctx->preset > 0)
	{
		ctx->preset--;
		x264_param_default_preset(&ctx->param,
					  x264_preset_names[ctx->preset],
					"zerolatency");
		SDL_Log("Reduce preset to %s", x264_preset_names[ctx->preset]);
	}
	else if((now_ms - ctx->delay_ms) < ctx->min_delay_ms &&
			ctx->preset < preset_max)
	{
		ctx->preset++;
		x264_param_default_preset(&ctx->param,
					  x264_preset_names[ctx->preset],
					"zerolatency");
		SDL_Log("Increased preset to %s", x264_preset_names[ctx->preset]);
	}

	ctx->delay_ms = now_ms;
#endif

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
		SDL_RWwrite(ctx->f, nal[i].p_payload, nal[i].i_payload, 1);

ret:
	ret = 0;
err:
	SDL_AtomicUnlock(&ctx->lock);
	SDL_FreeSurface(framedat->surf);
	SDL_free(data);
	return ret;
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

static Uint32 allow_speedmod(Uint32 interval, void *param)
{
	enc_vid *ctx = param;
	(void) interval;
	SDL_AtomicSet(&ctx->speedmod_timeout, 0);
	return 0;
}

void vid_enc_speedup(enc_vid *ctx)
{
	const float rfc_max = 30.0F;
	float rfc;

	if(ctx == NULL)
		return;

	if(ctx->preset == 0)
		return;

	SDL_AtomicLock(&ctx->lock);
	rfc = ctx->param.rc.f_rf_constant;

	ctx->preset--;
	x264_param_default_preset(&ctx->param, x264_preset_names[ctx->preset],
				  "zerolatency");

#if 0
	if(rfc < rfc_max)
		ctx->param.rc.f_rf_constant = rfc + 1.0F;
#endif

	x264_encoder_reconfig(ctx->h, &ctx->param);
	SDL_LogVerbose(SDL_LOG_CATEGORY_VIDEO, "Modified video preset to %s",
				x264_preset_names[ctx->preset]);

	SDL_AtomicUnlock(&ctx->lock);
}

void vid_enc_speeddown(enc_vid *ctx)
{
	const float rfc_min = 15.0F;
	float rfc;

	if(ctx == NULL)
		return;

	if(ctx->preset == preset_max)
		return;

	if(SDL_AtomicGet(&ctx->speedmod_timeout))
		return;

	SDL_AtomicSet(&ctx->speedmod_timeout, 1);
	SDL_AddTimer(100, allow_speedmod, ctx);

	SDL_AtomicLock(&ctx->lock);
	rfc = ctx->param.rc.f_rf_constant;

	ctx->preset++;
	x264_param_default_preset(&ctx->param, x264_preset_names[ctx->preset],
				  "zerolatency");

#if 0
	if(ctx->param.rc.f_rf_constant > rfc_min)
		ctx->param.rc.f_rf_constant = rfc - 1.0F;
#endif

	x264_encoder_reconfig(ctx->h, &ctx->param);
	SDL_LogVerbose(SDL_LOG_CATEGORY_VIDEO, "Modified video preset to %s",
				x264_preset_names[ctx->preset]);
	SDL_AtomicUnlock(&ctx->lock);
}

void vid_enc_end(enc_vid *ctx)
{
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
			SDL_RWwrite(ctx->f, nal[i].p_payload, nal[i].i_payload, 1);
	}

	x264_encoder_close(ctx->h);

out:
	SDL_RWclose(ctx->f);
	SDL_AtomicUnlock(&ctx->lock);
	SDL_free(ctx);
	return;
}

