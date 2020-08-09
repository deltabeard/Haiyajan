#pragma once

#include <SDL.h>
#include <libretro.h>

typedef struct gl_ctx_s gl_ctx;

/**
 * Allocate OpenGL context.
 */
gl_ctx *gl_prepare(SDL_Renderer *rend);

/**
 * Initialise OpenGL context for the given texture.
 */
int gl_init(gl_ctx *ctx, SDL_Texture **tex,
	    struct retro_hw_render_callback *lrhw);

/**
 * Resets the OpenGL context.
 */
void gl_reset_context(const gl_ctx *const ctx);

/**
 * To be called before retro_run() takes place when OpenGL calls are to be made
 * by the loaded core.
 */
void gl_prerun(gl_ctx *ctx);

/**
 * To be called after retro_run() takes place to allow SDL2 to draw to the
 * screen properly.
 */
void gl_postrun(gl_ctx *ctx);

/**
 * Free the OpenGL context.
 */
void gl_deinit(gl_ctx *ctx);
