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

#define MAX_PLAYERS 4

/* The input type connected to Haiyajan and to the Libretro core. */
enum input_type_e {
	/* Any unused device is set to NONE. */
	RETRO_INPUT_NONE =	RETRO_DEVICE_NONE,

	/* Any game controller without analogue input. */
	RETRO_INPUT_JOYPAD =	RETRO_DEVICE_JOYPAD,

	RETRO_INPUT_MOUSE =	RETRO_DEVICE_MOUSE,
	RETRO_INPUT_KEYBOARD =	RETRO_DEVICE_KEYBOARD,
	RETRO_INPUT_LIGHTGUN =	RETRO_DEVICE_LIGHTGUN,

	/* Any game controller with analogue inputs. */
	RETRO_INPUT_ANALOG =	RETRO_DEVICE_ANALOG,

	RETRO_INPUT_POINTER =	RETRO_DEVICE_POINTER
};
typedef enum input_type_e input_type;

/* The type of command executed by the input button press. */
enum input_cmd_type_e {
	INPUT_CMD_NONE = 0,
	INPUT_CMD_RETRO_INPUT,
	INPUT_CMD_CALL_FUNC
};
typedef enum input_cmd_type_e input_cmd_type;

/* The commands that may be mapped to input button press. */
enum input_cmd_event_codes_e {
	INPUT_EVENT_TOGGLE_INFO = 0,
	INPUT_EVENT_TOGGLE_FULLSCREEN,
	INPUT_EVENT_TAKE_SCREENCAPTURE
};
typedef enum input_cmd_event_codes_e input_cmd_event;

/* Libretro joypad input as an enum for improved type tracking. */
enum input_cmd_joypad_codes_e {
	INPUT_JOYPAD_B = RETRO_DEVICE_ID_JOYPAD_B,
	INPUT_JOYPAD_Y,
	INPUT_JOYPAD_SELECT,
	INPUT_JOYPAD_START,
	INPUT_JOYPAD_UP,
	INPUT_JOYPAD_DOWN,
	INPUT_JOYPAD_LEFT,
	INPUT_JOYPAD_RIGHT,
	INPUT_JOYPAD_A,
	INPUT_JOYPAD_X,
	INPUT_JOYPAD_L,
	INPUT_JOYPAD_R,
	INPUT_JOYPAD_L2,
	INPUT_JOYPAD_R2,
	INPUT_JOYPAD_L3,
	INPUT_JOYPAD_R3
};
typedef enum input_cmd_joypad_codes_e input_cmd_joypad;

/* The command to execute on button press. This depends on the input_cmd_type of
 * the mapping. */
union input_cmd_u {
	input_cmd_event event;
	input_cmd_joypad joypad;
	unsigned char byte;
};

union input_cmd_trigger_u {
	SDL_GameControllerButton btn;
	//SDL_GameControllerAxis axis;
	SDL_Scancode sc;
};

struct input_device_s {
	/* Type of device connected to Haiyajan. */
	input_type hai_type;

	/* Type of device connected to Libretro core. */
	input_type lr_type;

#if 0
	/* Player number. Undefined if lr_type is RETRO_INPUT_NONE. */
	Uint8 player;
#endif

	/* State of all the retro_device buttons.
	 * Where bit 0 is RETRO_DEVICE_ID_JOYPAD_B. */
	Uint16 retro_state;

	/* Pointer to gamecontroller if type is INPUT_TYPE_CONTROLLER*. */
	SDL_GameController *gc;
	/* Game Controller mapping if type is INPUT_TYPE_CONTROLLER*. */
	//Uint8 gcbtn[SDL_CONTROLLER_BUTTON_MAX];
	/* Additional mappings if the controller has axis input, and hence is a
	 * INPUT_TYPE_CONTROLLER_ANALOGUE only. */
	//Sint16 gcax[SDL_CONTROLLER_AXIS_MAX];
};

struct input_ctx_s
{
	Uint32 input_cmd_event;
	struct input_device_s player[MAX_PLAYERS];
};

#define INPUT_EVENT_CHK(x) (x >= 0x300 && x < 0x900)

void input_init(struct input_ctx_s *restrict in_ctx);
void input_handle_event(struct input_ctx_s *const in_ctx, const SDL_Event *ev);
Sint16 input_get(const struct input_ctx_s *const in_ctx,
		 unsigned port, unsigned device, unsigned index, unsigned id);
