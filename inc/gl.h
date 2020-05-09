
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <libretro.h>

struct gl_ctx_s;
typedef struct gl_ctx_s glctx;

glctx *gl_init(SDL_Renderer *rend, SDL_Texture *tex,
			   struct retro_hw_render_callback *lrhw);
void gl_prerun(glctx *ctx);
void gl_postrun(glctx *ctx, const SDL_Rect *screen_dim);
void gl_deinit(glctx *ctx);
void gl_set_texture(glctx *ctx, SDL_Texture *tex);
