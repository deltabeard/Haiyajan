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

#include <SDL.h>
#include <libretro.h>

#define MAX_PLAYERS 4

/* The input type connected to Haiyajan and to the Libretro core. */
typedef enum {
	/* Any unused device is set to NONE. */
	RETRO_INPUT_NONE =	RETRO_DEVICE_NONE,

	/* Any game controller without analogue input. */
	RETRO_INPUT_JOYPAD =	RETRO_DEVICE_JOYPAD,

	RETRO_INPUT_MOUSE =	RETRO_DEVICE_MOUSE,
	RETRO_INPUT_KEYBOARD =	RETRO_DEVICE_KEYBOARD,
	RETRO_INPUT_LIGHTGUN =	RETRO_DEVICE_LIGHTGUN,

	/* Any game controller with analogue inputs. */
	RETRO_INPUT_ANALOG =	RETRO_DEVICE_ANALOG,

	RETRO_INPUT_POINTER =	RETRO_DEVICE_POINTER,

	RETRO_INPUT_MAX
} input_type_e;

/* The type of command executed by the input button press. */
typedef enum {
	INPUT_CMD_NONE = 0,
	INPUT_CMD_INPUT,
	INPUT_CMD_EVENT,
	INPUT_CMD_RESERVED
} input_cmd_type_e;

/* The commands that may be mapped to input button press. */
typedef enum {
	INPUT_EVENT_TOGGLE_INFO = 0,
	INPUT_EVENT_TOGGLE_FULLSCREEN,
	INPUT_EVENT_TAKE_SCREENSHOT,
	INPUT_EVENT_RECORD_VIDEO_TOGGLE
} input_cmd_event_codes_e;

/* Libretro joypad input as an enum for improved type tracking. */
typedef enum {
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
	INPUT_JOYPAD_R3,

	INPUT_ANALOGUE_LEFT_X_POS,
	INPUT_ANALOGUE_LEFT_X_NEG,
	INPUT_ANALOGUE_LEFT_Y_POS,
	INPUT_ANALOGUE_LEFT_Y_NEG,
	INPUT_ANALOGUE_RIGHT_X_POS,
	INPUT_ANALOGUE_RIGHT_X_NEG,
	INPUT_ANALOGUE_RIGHT_Y_POS,
	INPUT_ANALOGUE_RIGHT_Y_NEG,
	INPUT_ANALOGUE_BTN
} input_cmd_input_e;

struct input_joypad_btns_s
{
	union
	{
		Uint16 btns;
		struct
		{
			unsigned input_joypad_btns_b : 1;
			unsigned input_joypad_btns_y : 1;
			unsigned input_joypad_btns_select : 1;
			unsigned input_joypad_btns_start : 1;
			unsigned input_joypad_btns_up : 1;
			unsigned input_joypad_btns_down : 1;
			unsigned input_joypad_btns_left : 1;
			unsigned input_joypad_btns_right : 1;
			unsigned input_joypad_btns_a : 1;
			unsigned input_joypad_btns_x : 1;
			unsigned input_joypad_btns_l : 1;
			unsigned input_joypad_btns_r : 1;
			unsigned input_joypad_btns_l2 : 1;
			unsigned input_joypad_btns_r2 : 1;
			unsigned input_joypad_btns_l3 : 1;
			unsigned input_joypad_btns_r3 : 1;
		} btns_bits;
	};
};

struct input_device_s {
	/* Type of device connected to Haiyajan. */
	input_type_e hai_type;

	/* Compatible input controllers with libretro core.
	 * Set bit: Input type compatible.
	 * Unset bit: Input type not compatible.
	 *
	 * 0bxx000000
	 *     │││││╰──── RETRO_INPUT_JOYPAD
	 *     ││││╰───── RETRO_INPUT_MOUSE
	 *     │││╰────── RETRO_INPUT_KEYBOARD
	 *     ││╰─────── RETRO_INPUT_LIGHTGUN
	 *     │╰──────── RETRO_INPUT_ANALOG
	 *     ╰───────── RETRO_INPUT_POINTER
	 */
	Uint8 available_types;

	/* Values within this union depend on the input_type_e hai_type. */
	union {
		struct {
			SDL_GameController *ctx;
			struct input_joypad_btns_s btns;
			Sint16 left_x, left_y;
			Sint16 right_x, right_y;
			Sint16 l2_x, r2_x;
		} pad;
		struct {
			struct input_joypad_btns_s btns;
			Sint16 x, y;
		} mouse;
		struct {
			struct input_joypad_btns_s btns;
			Sint16 left_x, left_y;
			Sint16 right_x, right_y;
		} keyboard;
		struct {
			Sint16 x, y;
			unsigned on_screen : 1;
			unsigned trigger : 1;
			unsigned select : 1;
			unsigned start : 1;
			unsigned a : 1;
			unsigned b : 1;
			unsigned c : 1;
			unsigned d : 1;
			unsigned dpad_up : 1;
			unsigned dpad_down : 1;
			unsigned dpad_left : 1;
			unsigned dpad_right : 1;
		} lightgun;
		struct {
			/* Number of presses on screen. */
			Uint8 pressed;

			/* Array of coordinates of "pressed" length, else NULL.
			 */
			struct touch_coords_s {
				Sint16 x, y;
			} *touch_coords;
		} pointer;
	};
};

struct input_ctx_s
{
	Uint32 input_cmd_event;
	struct input_device_s player[MAX_PLAYERS];
};

/**
 * Range of events that can be handled by input_handle_event().
 */
#define INPUT_EVENT_CHK(x) (x >= 0x300 && x < 0x900)

/**
 * Initialise input system.
 *
 * \param in_ctx	Input structure context to initialise.
 */
void input_init(struct input_ctx_s *restrict in_ctx);

/**
 * Set compatible controller information.
 *
 * \param ctx		Input module context.
 * \param port		Port or player number for input device.
 * \param device	Type of input device.
 */
void input_add_controller(struct input_ctx_s *ctx, unsigned port,
			  input_type_e device);

/**
 * Handle an input event.
 *
 * \param in_ctx	Input structure related to the event.
 * \param ev		Event to handle.
 */
void input_handle_event(struct input_ctx_s *const in_ctx, const SDL_Event *ev);

/**
 * Obtain input.
 *
 * \param in_ctx	Input struct context.
 * \param port		Port the controller is attached to.
 * \param device	Type of device that is connect to the port.
 * \param index		The series of buttons that is requested.
 * \param id		The button requested.
 * \return		Value of requested button. 0 is not pressed.
 */
Sint16 input_get(const struct input_ctx_s *const in_ctx,
		 unsigned port, unsigned device, unsigned index, unsigned id);


