#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <libretro.h>

struct gl_ctx_s;
typedef struct gl_ctx_s glctx;

glctx *gl_init(SDL_Renderer *rend, SDL_Texture **tex,
	       struct retro_hw_render_callback *lrhw);
void gl_reset_context(const glctx *const ctx);
void gl_prerun(glctx *ctx);
void gl_postrun(glctx *ctx);
void gl_deinit(glctx *ctx);
