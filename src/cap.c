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
};

static void x264_log(void *priv, int i_level, const char *fmt, va_list ap)
{
	const SDL_LogPriority lvlmap[] = {
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

	/* Get default params for preset/tuning.
	 * Setting preset to veryfast in order to reduce strain during gameplay. */
	if(x264_param_default_preset(&ctx->param, "superfast", "zerolatency") < 0)
		goto err;

	/* Configure non-default params */
	ctx->param.i_bitdepth = 8;
	ctx->param.i_csp = X264_CSP_RGB;
	ctx->param.vui.i_sar_width = ctx->param.i_width  = width;
	ctx->param.vui.i_sar_height = ctx->param.i_height = height;
	ctx->param.b_vfr_input = 0;
	ctx->param.b_repeat_headers = 1;
	ctx->param.b_annexb = 1;
	ctx->param.i_threads = 1;
	ctx->param.b_sliced_threads = 1;
	ctx->param.i_lookahead_threads = X264_THREADS_AUTO;
	ctx->param.pf_log = x264_log;

	ctx->param.rc.i_rc_method = X264_RC_CRF;
	ctx->param.rc.f_rf_constant = 14;
	ctx->param.rc.f_rf_constant_max = 35;

	SDL_assert(fps < 256.0);
	ctx->param.i_fps_num = (uint32_t)(fps * 16777216.0);
	ctx->param.i_fps_den = 16777216;
	ctx->param.i_timebase_den = ctx->param.i_fps_num;
	ctx->param.i_timebase_num = ctx->param.i_fps_den;

	ctx->param.vui.b_fullrange = 1;

	/* Apply profile restrictions. */
	if(x264_param_apply_profile(&ctx->param, "high444") < 0)
		goto err;

	ctx->h = x264_encoder_open(&ctx->param);
	if(ctx->h == NULL)
		goto err;

	x264_encoder_parameters( ctx->h, &ctx->param );

	ctx->f = SDL_RWFromFile(fileout, "wb");
	if(ctx->f == NULL)
		goto err;

#if 1
	x264_nal_t *headers;
	int i_nal;
	int bytes;
	if((bytes = x264_encoder_headers(ctx->h, &headers, &i_nal)) < 1)
	{
		SDL_SetError("Enable to obtain encoder headers");
		goto err;
	}

	if(SDL_RWwrite(ctx->f, headers, bytes, 1) == 0)
		goto err;
#endif

	ctx->frame = 0;
out:
	return ctx;

err:
	SDL_free(ctx);
	ctx = NULL;
	goto out;
}

int vid_enc_frame(enc_vid *ctx, SDL_Surface *surf)
{
	int ret = 1;
	int i_nal;
	int i_frame_size;
	x264_picture_t pic;
	x264_picture_t pic_out;
	x264_nal_t *nal;

	SDL_assert(surf->format->format == SDL_PIXELFORMAT_RGB24);

	if(x264_picture_alloc(&pic, ctx->param.i_csp, ctx->param.i_width,
			ctx->param.i_height) < 0)
		goto err;

	pic.img.i_csp = X264_CSP_RGB;
	pic.i_type = X264_TYPE_AUTO;
	pic.i_qpplus1 = X264_QP_AUTO;
	pic.img.i_plane = 1;
	pic.img.i_stride[0] = surf->pitch;
	pic.img.plane[0] = surf->pixels;
	i_frame_size = x264_encoder_encode(ctx->h, &nal, &i_nal, &pic,
					   &pic_out);
	if(i_frame_size < 0)
	{
		SDL_SetError("x264: unable to encode frame");
		goto err;
	}

	/* Increment frame number. */
	pic.i_pts = ctx->frame;
	ctx->frame++;

	if(i_frame_size == 0)
		goto ret;

	if(SDL_RWwrite(ctx->f, &nal->p_payload, i_frame_size, 1) == 0)
		goto err;

ret:
	ret = 0;
err:
	return ret;
}

void vid_enc_end(enc_vid *ctx)
{
	/* Flush delayed frames */
	while(x264_encoder_delayed_frames(ctx->h))
	{
		int i_nal;
		x264_nal_t *nal;
		x264_picture_t pic_out;
		int i_frame_size = x264_encoder_encode(ctx->h, &nal,
						       &i_nal, NULL,
							&pic_out);
		if(i_frame_size < 0)
			goto out;

		if(i_frame_size == 0)
			continue;

		if(SDL_RWwrite(ctx->f, &nal->p_payload, i_frame_size, 1) == 0)
			goto out;
	}

	x264_encoder_close(ctx->h);
	//x264_picture_clean(&ctx->pic);
	SDL_RWclose(ctx->f);
	SDL_free(ctx);

out:
	return;
}
