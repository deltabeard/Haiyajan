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

struct keymap_info_s {
	/* A value of type input_cmd_type. */
	unsigned char input_cmd_type : 2;

	/* Hold either input_cmd_event or input_cmd_joypad depending on
	 * input_cmd_type. */
	unsigned char input_cmd : 6;
	/*
	union input_cmd_u input_cmd;
	*/
};

/* It is assumed that the user only has one keyboard in use. This one keyboard
 * may be used to control any number of players. */
/* TODO: Set default keymapping here. */
static struct keymap_info_s keymap[SDL_NUM_SCANCODES] = { 0 };

/* Forward declarations. */
static void input_set(struct input_ctx_s *const in_ctx, SDL_Scancode sc,
		      Uint8 state);
void input_toggle_ui_info(struct input_ctx_s *const in_ctx);
void input_toggle_fullscreen(struct input_ctx_s *const in_ctx);

static const char *const lr_input_string[] = {
	"None", "Joypad", "Mouse", "Keyboard", "Lightgun", "Analogue", "Pointer"
};

#if 0
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
#endif

void input_toggle_ui_info(struct input_ctx_s *const in_ctx)
{
	SDL_Event event = { 0 };
	event.type = in_ctx->input_cmd_event;
	event.user.code = INPUT_EVENT_TOGGLE_INFO;
	SDL_PushEvent(&event);
}

void input_toggle_fullscreen(struct input_ctx_s *const in_ctx)
{
	SDL_Event event = { 0 };
	event.type = in_ctx->input_cmd_event;
	event.user.code = INPUT_EVENT_TOGGLE_FULLSCREEN;
	SDL_PushEvent(&event);
}

void input_map(struct input_ctx_s *const in_ctx, input_type input_type,
	       union input_cmd_trigger_u trig,
	       const struct keymap_info_s *const map)
{
	(void)in_ctx;
	if(input_type == INPUT_TYPE_KEYBOARD)
	{
		keymap[trig.sc] = *map;
	}
	else
	{
		SDL_LogWarn(SDL_LOG_CATEGORY_INPUT,
			    "Unable to map command to unsupported input type");
	}
}

void input_init(struct input_ctx_s *restrict in_ctx)
{
	#include <gamecontrollerdb.h>
	SDL_RWops *gcdb_rw;
	const struct {
		union input_cmd_trigger_u trig;
		struct keymap_info_s map;
	} keymap_defaults[] = {
		{
			{ .sc = SDL_SCANCODE_X },
			{
				.input_cmd_type = INPUT_CMD_RETRO_INPUT,
				.input_cmd = INPUT_JOYPAD_B
			}
		},
		{{ .sc = SDL_SCANCODE_S },	{ INPUT_CMD_RETRO_INPUT, INPUT_JOYPAD_Y }},
		{{ .sc = SDL_SCANCODE_BACKSPACE},{INPUT_CMD_RETRO_INPUT, INPUT_JOYPAD_Y }},
		{{ .sc = SDL_SCANCODE_RETURN },	{ INPUT_CMD_RETRO_INPUT, INPUT_JOYPAD_Y }},
		{{ .sc = SDL_SCANCODE_UP},	{ INPUT_CMD_RETRO_INPUT, INPUT_JOYPAD_Y }},
		{{ .sc = SDL_SCANCODE_DOWN},	{ INPUT_CMD_RETRO_INPUT, INPUT_JOYPAD_Y }},
		{{ .sc = SDL_SCANCODE_LEFT},	{ INPUT_CMD_RETRO_INPUT, INPUT_JOYPAD_Y }},
		{{ .sc = SDL_SCANCODE_RIGHT},	{ INPUT_CMD_RETRO_INPUT, INPUT_JOYPAD_Y }},
		{{ .sc = SDL_SCANCODE_Z},	{ INPUT_CMD_RETRO_INPUT, INPUT_JOYPAD_Y }},
		{{ .sc = SDL_SCANCODE_A},	{ INPUT_CMD_RETRO_INPUT, INPUT_JOYPAD_Y }},
		{{ .sc = SDL_SCANCODE_Q},	{ INPUT_CMD_RETRO_INPUT, INPUT_JOYPAD_Y }},
		{{ .sc = SDL_SCANCODE_W},	{ INPUT_CMD_RETRO_INPUT, INPUT_JOYPAD_Y }},
		{{ .sc = SDL_SCANCODE_E},	{ INPUT_CMD_RETRO_INPUT, INPUT_JOYPAD_Y }},
		{{ .sc = SDL_SCANCODE_R},	{ INPUT_CMD_RETRO_INPUT, INPUT_JOYPAD_Y }},
		{{ .sc = SDL_SCANCODE_T},	{ INPUT_CMD_RETRO_INPUT, INPUT_JOYPAD_Y }},
		{{ .sc = SDL_SCANCODE_Y},	{ INPUT_CMD_RETRO_INPUT, INPUT_JOYPAD_Y }},
		{{ .sc = SDL_SCANCODE_I},	{ INPUT_CMD_CALL_FUNC,   INPUT_EVENT_TOGGLE_INFO }},
		{{ .sc = SDL_SCANCODE_F},	{ INPUT_CMD_RETRO_INPUT, INPUT_EVENT_TOGGLE_FULLSCREEN }}
	};

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

	/* Set default keyboard keymap. */
	for(unsigned i = 0; i < SDL_arraysize(keymap_defaults); i++)
	{
		input_map(in_ctx, INPUT_TYPE_KEYBOARD, keymap_defaults[i].trig,
				  &keymap_defaults[i].map);
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
		const char *const no_name = "with no name";

		if(SDL_IsGameController(joy_ind) != SDL_TRUE)
		{
			const char *joy_name = SDL_JoystickNameForIndex(joy_ind);
			SDL_LogWarn(SDL_LOG_CATEGORY_INPUT,
				"The attached controller \"%s\" is not "
				"supported",
				joy_name == NULL ? no_name : joy_name);
			return;
		}

		gc_name = SDL_GameControllerNameForIndex(joy_ind);
		if(gc_name == NULL)
			gc_name = no_name;

		gc = SDL_GameControllerOpen(joy_ind);
		if(gc == NULL)
		{
			SDL_LogInfo(SDL_LOG_CATEGORY_INPUT,
				"Unable to open controller \"%s\": %s",
				gc_name, SDL_GetError());
			return;
		}

		in_ctx->player[0].type = INPUT_TYPE_CONTROLLER;
		in_ctx->player[0].lr_type = input_get_device_type(gc);
		in_ctx->player[0].player = 1;
		in_ctx->player[0].gc = gc;

		/* FIXME: assign controller mapping to core. */

		SDL_LogInfo(SDL_LOG_CATEGORY_INPUT,
			    "Controller \"%s\" connected as an %s device",
			    gc_name, lr_input_string[in_ctx->player[0].lr_type]);
	}
	else if(ev->type == SDL_CONTROLLERDEVICEREMAPPED)
	{
		SDL_LogVerbose(SDL_LOG_CATEGORY_INPUT, "Controller remapped");
	}
}

static void input_set(struct input_ctx_s *const in_ctx, SDL_Scancode sc,
		      Uint8 state)
{
	switch(keymap[sc].input_cmd_type)
	{
	case INPUT_CMD_RETRO_INPUT:
		in_ctx->player[0].retro_state[keymap[sc].input_cmd] = state;
		break;

	case INPUT_CMD_CALL_FUNC:
		if(in_ctx->input_cmd_event == (Uint32) - 1)
			break;

		if(state)
		{
			SDL_Event event = { 0 };
			event.type = in_ctx->input_cmd_event;
			event.user.code = keymap[sc].input_cmd;
			SDL_PushEvent(&event);
		}
		break;
	}
}

Sint16 input_get(struct input_ctx_s *in_ctx,
		 unsigned port, unsigned device, unsigned index, unsigned id)
{
	if(port != 0 || index != 0 || device != RETRO_DEVICE_JOYPAD ||
		id >= SDL_arraysize(keymap) || port >= MAX_PLAYERS)
		return 0;

	switch(device)
	{
		case RETRO_DEVICE_JOYPAD:
			return (Sint16) in_ctx->player[port].retro_state[id];

		//case RETRO_DEVICE_ANALOG:
	}
	return 0;
}
