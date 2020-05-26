#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <libretro.h>

struct gl_ctx_s;
typedef struct gl_ctx_s glctx;

/**
 * Initialise OpenGL context for the given texture.
 */
glctx *gl_init(SDL_Renderer *rend, SDL_Texture **tex,
	       struct retro_hw_render_callback *lrhw);

/**
 * Resets the OpenGL context.
 */
void gl_reset_context(const glctx *const ctx);

/**
 * To be called before retro_run() takes place when OpenGL calls are to be made
 * by the loaded core.
 */
void gl_prerun(glctx *ctx);

/**
 * To be called after retro_run() takes place to allow SDL2 to draw to the
 * screen properly.
 */
void gl_postrun(glctx *ctx);

/**
 * Free the OpenGL context.
 */
void gl_deinit(glctx *ctx);
