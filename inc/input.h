/**
 * Handles user input.
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

#pragma once

#include <SDL2/SDL.h>

struct input_ctx_s
{
	Sint16 joypad_state[16];
	Uint32 input_cmd_event;
};

enum input_cmd_event_codes_e {
	INP_EVNT_TOGGLE_INFO,
	INP_EVENT_TOGGLE_FULLSCREEN
};

typedef enum input_cmd_event_codes_e inp_evnt_code;

#define INPUT_EVENT_CHK(x) (x >= 0x300 && x < 0x900)

int input_init(struct input_ctx_s *in_ctx);
void input_handle_event(struct input_ctx_s *in_ctx, const SDL_Event *ev);
Sint16 input_get(struct input_ctx_s *in_ctx,
		 unsigned port, unsigned device, unsigned index, unsigned id);
