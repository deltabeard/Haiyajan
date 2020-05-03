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

#include <SDL2/SDL.h>
#include <libretro.h>
#include <input.h>
#include <gamecontrollerdb.h>

enum input_cmd_type_e {
	INPUT_CMD_RETRO_INPUT,
	INPUT_CMD_CALL_FUNC
};
typedef enum input_cmd_type_e input_cmd_type;

struct keymap_s
{
	SDL_Keycode sdl_keycode;

	union {
		SDL_GameControllerButton btn;
		SDL_GameControllerAxis ax;
	};

	const input_cmd_type in_type;

	const union
	{
		unsigned retro_id;
		void (*fn)(struct input_ctx_s *const);
	};
};

/* Forward declarations. */
static void input_set(struct input_ctx_s *in_ctx, SDL_Keycode code, Sint16 state);

static const char *const lr_input_string[] = {
	"None", "Joypad", "Mouse", "Keyboard", "Lightgun", "Analogue", "Pointer"
};

void input_toggle_ui_info(struct input_ctx_s *const in_ctx)
{
	SDL_Event event;
	SDL_memset(&event, 0, sizeof(event)); /* or SDL_zero(event) */
	event.type = in_ctx->input_cmd_event;
	event.user.code = INP_EVNT_TOGGLE_INFO;
	event.user.data1 = NULL;
	event.user.data2 = NULL;
	SDL_PushEvent(&event);
}

void input_toggle_fullscreen(struct input_ctx_s *const in_ctx)
{
	SDL_Event event;
	SDL_memset(&event, 0, sizeof(event)); /* or SDL_zero(event) */
	event.type = in_ctx->input_cmd_event;
	event.user.code = INP_EVENT_TOGGLE_FULLSCREEN;
	event.user.data1 = NULL;
	event.user.data2 = NULL;
	SDL_PushEvent(&event);
}

static const struct keymap_s keymap[] =
{
	{ SDLK_x,	{ .btn = SDL_CONTROLLER_BUTTON_B },		INPUT_CMD_RETRO_INPUT,	{ RETRO_DEVICE_ID_JOYPAD_B }	},
	{ SDLK_s,	SDL_CONTROLLER_BUTTON_Y,		INPUT_CMD_RETRO_INPUT,	{ RETRO_DEVICE_ID_JOYPAD_Y }	},
	{ SDLK_BACKSPACE, SDL_CONTROLLER_BUTTON_BACK,		INPUT_CMD_RETRO_INPUT,	{ RETRO_DEVICE_ID_JOYPAD_SELECT}},
	{ SDLK_RETURN,	SDL_CONTROLLER_BUTTON_START,		INPUT_CMD_RETRO_INPUT,	{ RETRO_DEVICE_ID_JOYPAD_START }},
	{ SDLK_UP,	SDL_CONTROLLER_BUTTON_DPAD_UP,		INPUT_CMD_RETRO_INPUT,	{ RETRO_DEVICE_ID_JOYPAD_UP }	},
	{ SDLK_DOWN,	SDL_CONTROLLER_BUTTON_DPAD_DOWN,	INPUT_CMD_RETRO_INPUT,	{ RETRO_DEVICE_ID_JOYPAD_DOWN }	},
	{ SDLK_LEFT,	SDL_CONTROLLER_BUTTON_DPAD_LEFT,	INPUT_CMD_RETRO_INPUT,	{ RETRO_DEVICE_ID_JOYPAD_LEFT }	},
	{ SDLK_RIGHT,	SDL_CONTROLLER_BUTTON_DPAD_RIGHT,	INPUT_CMD_RETRO_INPUT,	{ RETRO_DEVICE_ID_JOYPAD_RIGHT }},
	{ SDLK_z,	SDL_CONTROLLER_BUTTON_A,		INPUT_CMD_RETRO_INPUT,	{ RETRO_DEVICE_ID_JOYPAD_A }	},
	{ SDLK_a,	SDL_CONTROLLER_BUTTON_X,		INPUT_CMD_RETRO_INPUT,	{ RETRO_DEVICE_ID_JOYPAD_X }	},
	{ SDLK_q,	SDL_CONTROLLER_BUTTON_LEFTSHOULDER,	INPUT_CMD_RETRO_INPUT,	{ RETRO_DEVICE_ID_JOYPAD_L }	},
	{ SDLK_w,	SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,	INPUT_CMD_RETRO_INPUT,	{ RETRO_DEVICE_ID_JOYPAD_R }	},
	{ SDLK_e,	SDL_CONTROLLER_AXIS_TRIGGERLEFT,	INPUT_CMD_RETRO_INPUT,	{ RETRO_DEVICE_ID_JOYPAD_L2 }	},
	{ SDLK_r,	SDL_CONTROLLER_AXIS_TRIGGERRIGHT,	INPUT_CMD_RETRO_INPUT,	{ RETRO_DEVICE_ID_JOYPAD_R2 }	},
	{ SDLK_t,	SDL_CONTROLLER_BUTTON_LEFTSTICK,	INPUT_CMD_RETRO_INPUT,	{ RETRO_DEVICE_ID_JOYPAD_L3 }	},
	{ SDLK_y,	SDL_CONTROLLER_BUTTON_RIGHTSTICK,	INPUT_CMD_RETRO_INPUT,	{ RETRO_DEVICE_ID_JOYPAD_R3 }	},

	{ SDLK_i,	SDL_CONTROLLER_BUTTON_INVALID,		INPUT_CMD_CALL_FUNC,	{ .fn = input_toggle_ui_info }	},
	{ SDLK_f,	SDL_CONTROLLER_BUTTON_INVALID,		INPUT_CMD_CALL_FUNC,	{ .fn = input_toggle_fullscreen }}
};

void input_init(struct input_ctx_s *restrict in_ctx)
{
	SDL_RWops *gcdb_rw;

	SDL_zerop(in_ctx);
	gcdb_rw = SDL_RWFromConstMem(gamecontrollerdb_txt,
				     gamecontrollerdb_txt_len);
	if(SDL_GameControllerAddMappingsFromRW(gcdb_rw, SDL_TRUE) == -1)
	{
		SDL_LogWarn(SDL_LOG_CATEGORY_INPUT,
			    "Unable to load internal controller mappings: %s",
			    SDL_GetError());
	}

	if((in_ctx->input_cmd_event = SDL_RegisterEvents(1)) == (Uint32)-1)
	{
		SDL_LogWarn(SDL_LOG_CATEGORY_INPUT,
			"Special input commands will not be available: %s",
			SDL_GetError());
	}

	return;
}

static libretro_input_type input_get_device_type(SDL_GameController *gc)
{
	SDL_GameControllerAxis axis;

	/* If there's a single analogue input, set to analogue controller. */
	/* FIXME: Check if it matches an actual dualshock 2. */
	for(axis = 0; axis < SDL_CONTROLLER_AXIS_MAX; axis++)
	{
		SDL_GameControllerButtonBind bind =
			SDL_GameControllerGetBindForAxis(gc, axis);

		if(bind.bindType == SDL_CONTROLLER_BINDTYPE_AXIS)
			return RETRO_INPUT_ANALOG;
	}

	return RETRO_INPUT_JOYPAD;
}

void input_handle_event(struct input_ctx_s *in_ctx, const SDL_Event *ev)
{
	if(ev->type == SDL_KEYDOWN)
		input_set(in_ctx, ev->key.keysym.sym, SDL_PRESSED);
	else if(ev->type == SDL_KEYUP)
		input_set(in_ctx, ev->key.keysym.sym, SDL_RELEASED);
	else if(ev->type == SDL_CONTROLLERDEVICEADDED)
	{
		SDL_GameController *gc;
		const Sint32 joy_ind = ev->cdevice.which;
		const char *gc_name;
		const char *const gc_no_name = "Controller with no name";

		if(SDL_IsGameController(joy_ind) != SDL_TRUE)
		{
			const char *joy_name = SDL_JoystickNameForIndex(joy_ind);
			SDL_LogWarn(SDL_LOG_CATEGORY_INPUT,
				"The controller \"%s\" is not supported",
				joy_name);
			return;
		}

		gc_name = SDL_GameControllerNameForIndex(joy_ind);
		gc = SDL_GameControllerOpen(joy_ind);
		if(gc == NULL)
		{
			SDL_LogInfo(SDL_LOG_CATEGORY_INPUT,
				"Unable to open controller %s: %s",
				gc_name == NULL ? "with no name" : gc_name,
				SDL_GetError());
			return;
		}

		if(gc_name == NULL)
			gc_name = gc_no_name;

		in_ctx->device[0].type = INPUT_TYPE_CONTROLLER;
		in_ctx->device[0].lr_type = input_get_device_type(gc);
		in_ctx->device[0].player = 1;
		in_ctx->device[0].gc = gc;

		SDL_LogInfo(SDL_LOG_CATEGORY_INPUT,
			    "%s connected as an %s device",
			    gc_name, lr_input_string[in_ctx->device[0].lr_type]);
	}
	else if(ev->type == SDL_CONTROLLERDEVICEREMAPPED)
	{
		SDL_LogVerbose(SDL_LOG_CATEGORY_INPUT, "Controller remapped");
	}
}

static void input_set(struct input_ctx_s *in_ctx, SDL_Keycode code, Sint16 state)
{
	for(size_t i = 0; i < SDL_arraysize(keymap); i++)
	{
		if((Sint32)code != keymap[i].sdl_keycode)
			continue;

		switch(keymap[i].in_type)
		{
			case INPUT_CMD_RETRO_INPUT:
				in_ctx->joypad_state[i] = state;
				break;

			case INPUT_CMD_CALL_FUNC:
				if(in_ctx->input_cmd_event == (Uint32)-1)
					break;

				if(state)
					keymap[i].fn(in_ctx);

				break;
		}
		break;
	}
}

Sint16 input_get(struct input_ctx_s *in_ctx,
		 unsigned port, unsigned device, unsigned index, unsigned id)
{
	if(port != 0 || index != 0 || device != RETRO_DEVICE_JOYPAD ||
		id >= SDL_arraysize(keymap))
		return 0;

	switch(device)
	{
		case RETRO_DEVICE_JOYPAD:
			return in_ctx->joypad_state[id];

		case RETRO_DEVICE_ANALOG:
	}
	return 0;
}
