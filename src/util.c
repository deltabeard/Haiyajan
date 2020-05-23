/**
 * Various utility functions for Haiyajan.
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
#include <time.h>
#include <util.h>

void gen_filename(char filename[static 64], const char *core_name,
		  const char fmt[static 3])
{
	time_t now;
	struct tm *tmp;
	char time_str[32];

	now = time(NULL);
	if(now == (time_t) -1 || (tmp = localtime(&now)) == NULL ||
		strftime(time_str, sizeof(time_str), "%Y-%m-%d-%H%M%S", tmp) == 0)
	{
		SDL_snprintf(time_str, sizeof(time_str), "%010u",
					 SDL_GetTicks());
	}

	SDL_snprintf(filename, 64, "%.32s%.1s%.20s.%s", time_str,
			core_name == NULL ? "" : "-",
			core_name == NULL ? "" : core_name,
			fmt);
}

struct at_tim_s
{
	Uint32 timeout_ms;
	SDL_atomic_t *atomic;
	int setval;
};

static SDL_atomic_t util_exit_threads;

static int util_timeout_thread(void *param)
{
	struct at_tim_s *at = param;

	SDL_SetThreadPriority(SDL_THREAD_PRIORITY_LOW);
	SDL_Delay(at->timeout_ms);

	if(SDL_AtomicGet(&util_exit_threads) == 0)
		goto out;

	SDL_AtomicSet(at->atomic, at->setval);

out:
	SDL_free(at);
	return 0;
}

void set_atomic_timeout(Uint32 timeout_ms, SDL_atomic_t *atomic, int setval,
	const char *name)
{
	SDL_Thread *t;
	struct at_tim_s *at = SDL_malloc(sizeof(struct at_tim_s));

	if(at == NULL)
	{
		SDL_AtomicSet(atomic, setval);
		return;
	}

	SDL_AtomicSet(&util_exit_threads, 1);
	at->timeout_ms = timeout_ms;
	at->atomic = atomic;
	at->setval = setval;

	t = SDL_CreateThread(util_timeout_thread, name, at);
	SDL_DetachThread(t);
}

void util_exit_all(void)
{
	SDL_AtomicSet(&util_exit_threads, 0);
	return;
}

SDL_Surface *util_tex_to_surf(SDL_Renderer *rend, SDL_Texture *tex,
			      const SDL_Rect *const src,
			      const SDL_RendererFlip flip)
{
	SDL_Texture *core_tex;
	SDL_Surface *surf = NULL;
	int fmt = SDL_PIXELFORMAT_RGB24;
	/* TODO: Use native format of renderer. */

	if(src->w <= 0 || src->h <= 0)
		return NULL;

	core_tex = SDL_CreateTexture(rend, fmt, SDL_TEXTUREACCESS_TARGET,
				    src->w, src->h);
	if(core_tex == NULL)
		return NULL;

	if(SDL_SetRenderTarget(rend, core_tex) != 0)
		goto err;

	/* This fixes a bug whereby OpenGL cores appear as a white screen in the
	 * screencap. */
	SDL_RenderDrawPoint(rend, 0, 0);

	if(SDL_RenderCopyEx(rend, tex, src, src, 0.0, NULL, flip) != 0)
		goto err;

	surf = SDL_CreateRGBSurfaceWithFormat(0, src->w, src->h,
					      SDL_BITSPERPIXEL(fmt), fmt);
	if(surf == NULL)
		goto err;

	/* TODO: Convert format (if required) in new thread. */
	if(SDL_RenderReadPixels(rend, src, fmt, surf->pixels, surf->pitch) != 0)
	{
		SDL_FreeSurface(surf);
		goto err;
	}

err:
	SDL_SetRenderTarget(rend, NULL);
	SDL_DestroyTexture(core_tex);
	return surf;
}
