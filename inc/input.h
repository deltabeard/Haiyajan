#pragma once

#include <SDL2/SDL.h>
#include <haiyajan.h>

void input_init(struct input_ctx_s *in_ctx, struct cmd_args_s *const args);
void input_set(struct input_ctx_s *in_ctx, SDL_KeyCode code, Sint16 state);
Sint16 input_get(struct input_ctx_s *in_ctx,
		 unsigned port, unsigned device, unsigned index, unsigned id);
