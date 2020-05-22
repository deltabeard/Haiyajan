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

	SDL_snprintf(filename, 64, "%s-%s.%s", time_str, core_name, fmt);
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

SDL_Surface *util_tex_to_surf(SDL_Renderer *rend, SDL_Texture *tex)
{
	/* Get native texture format. */
	/* Usual stuff. */
}
