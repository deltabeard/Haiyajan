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
#include <libretro.h>

/* Input types used in Haiyajan. */
enum input_type_e {
	/* No controller connected to device or mapped to content. */
	INPUT_TYPE_DISCONNECTED,

	/* Keyboards can be mapped to RETRO_DEVICE_{NONE, JOYPAD, KEYBOARD}. */
	INPUT_TYPE_KEYBOARD,

	/* Game controllers can be mapped to
	 * RETRO_DEVICE_{NONE, JOYPAD, ANALOG } */
	INPUT_TYPE_CONTROLLER
};
typedef enum input_type_e input_type;

/* Input types used in libretro. */
enum libretro_input_type_e {
	RETRO_INPUT_NONE =	RETRO_DEVICE_NONE,
	RETRO_INPUT_JOYPAD =	RETRO_DEVICE_JOYPAD,
	RETRO_INPUT_MOUSE =	RETRO_DEVICE_MOUSE,
	RETRO_INPUT_KEYBOARD =	RETRO_DEVICE_KEYBOARD,
	RETRO_INPUT_LIGHTGUN =	RETRO_DEVICE_LIGHTGUN,
	RETRO_INPUT_ANALOG =	RETRO_DEVICE_ANALOG,
	RETRO_INPUT_POINTER =	RETRO_DEVICE_POINTER
};
typedef enum libretro_input_type_e libretro_input_type;

struct input_device_s {
	/* Type of SDL2 device connected to Haiyajan. */
	input_type type;

	/* Type of device connected to the core. */
	libretro_input_type lr_type;

	/* Player number. Undefined if lr_type is NONE. */
	unsigned player;

	/* Pointer to gamecontroller if type is INPUT_TYPE_CONTROLLER. */
	SDL_GameController *gc;
};

struct input_ctx_s
{
	Sint16 joypad_state[16];
	Uint32 input_cmd_event;

	struct input_device_s device[4];
};

enum input_cmd_event_codes_e {
	INP_EVNT_TOGGLE_INFO,
	INP_EVENT_TOGGLE_FULLSCREEN
};

typedef enum input_cmd_event_codes_e inp_evnt_code;

#define INPUT_EVENT_CHK(x) (x >= 0x300 && x < 0x900)

void input_init(struct input_ctx_s *restrict in_ctx);
void input_handle_event(struct input_ctx_s *in_ctx, const SDL_Event *ev);
Sint16 input_get(struct input_ctx_s *in_ctx,
		 unsigned port, unsigned device, unsigned index, unsigned id);
