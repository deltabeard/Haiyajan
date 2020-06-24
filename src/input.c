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
#include <tinf.h>

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

static const char *const input_type_str[] = {
	"None", "Joypad", "Mouse", "Keyboard", "Lightgun", "Analogue", "Pointer"
};

void input_map(struct input_ctx_s *const in_ctx, input_type input_type,
	       union input_cmd_trigger_u trig,
	       const struct keymap_info_s *const map)
{
	(void)in_ctx;
	if(input_type == RETRO_INPUT_JOYPAD ||
		input_type == RETRO_INPUT_KEYBOARD)
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
#if defined(__linux__)
	#include <gcdb_bin_linux.h>
#elif defined(_WIN32)
	#include <gcdb_bin_windows.h>
#else
	#include <gcdb_bin_all.h>
#endif
	SDL_RWops *gcdb_rw;
	Uint8 *gcdb_txt;
	size_t gcdb_txt_len_act = gcdb_txt_len;
	tinf_error_code tinf;
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
		{{ .sc = SDL_SCANCODE_X },	{ INPUT_CMD_RETRO_INPUT, INPUT_JOYPAD_B }},
		{{ .sc = SDL_SCANCODE_S },	{INPUT_CMD_RETRO_INPUT, INPUT_JOYPAD_Y }},
		{{ .sc = SDL_SCANCODE_RETURN },	{ INPUT_CMD_RETRO_INPUT, INPUT_JOYPAD_SELECT }},
		{{ .sc = SDL_SCANCODE_BACKSPACE},{ INPUT_CMD_RETRO_INPUT, INPUT_JOYPAD_START }},
		{{ .sc = SDL_SCANCODE_UP },	{ INPUT_CMD_RETRO_INPUT, INPUT_JOYPAD_UP }},
		{{ .sc = SDL_SCANCODE_DOWN },	{ INPUT_CMD_RETRO_INPUT, INPUT_JOYPAD_DOWN }},
		{{ .sc = SDL_SCANCODE_LEFT },	{ INPUT_CMD_RETRO_INPUT, INPUT_JOYPAD_LEFT }},
		{{ .sc = SDL_SCANCODE_RIGHT },	{ INPUT_CMD_RETRO_INPUT, INPUT_JOYPAD_RIGHT }},
		{{ .sc = SDL_SCANCODE_Z },	{ INPUT_CMD_RETRO_INPUT, INPUT_JOYPAD_A }},
		{{ .sc = SDL_SCANCODE_A },	{ INPUT_CMD_RETRO_INPUT, INPUT_JOYPAD_X }},
		{{ .sc = SDL_SCANCODE_Q },	{ INPUT_CMD_RETRO_INPUT, INPUT_JOYPAD_L }},
		{{ .sc = SDL_SCANCODE_W },	{ INPUT_CMD_RETRO_INPUT, INPUT_JOYPAD_R }},
		{{ .sc = SDL_SCANCODE_E },	{ INPUT_CMD_RETRO_INPUT, INPUT_JOYPAD_L2 }},
		{{ .sc = SDL_SCANCODE_R },	{ INPUT_CMD_RETRO_INPUT, INPUT_JOYPAD_R2 }},
		{{ .sc = SDL_SCANCODE_T },	{ INPUT_CMD_RETRO_INPUT, INPUT_JOYPAD_L3 }},
		{{ .sc = SDL_SCANCODE_Y },	{ INPUT_CMD_RETRO_INPUT, INPUT_JOYPAD_R3 }},
		{{ .sc = SDL_SCANCODE_I },	{ INPUT_CMD_CALL_FUNC,   INPUT_EVENT_TOGGLE_INFO }},
		{{ .sc = SDL_SCANCODE_F },	{ INPUT_CMD_CALL_FUNC, INPUT_EVENT_TOGGLE_FULLSCREEN }},
		{{ .sc = SDL_SCANCODE_P },	{ INPUT_CMD_CALL_FUNC, INPUT_EVENT_TAKE_SCREENSHOT }},
		{{ .sc = SDL_SCANCODE_V },	{ INPUT_CMD_CALL_FUNC, INPUT_EVENT_RECORD_VIDEO_TOGGLE }}
	};

	SDL_zerop(in_ctx);

	gcdb_txt = SDL_malloc(gcdb_txt_len);
	if(gcdb_txt == NULL)
		goto err;

	tinf = tinf_uncompress(gcdb_txt, &gcdb_txt_len_act, gcdb_bin, gcdb_bin_len);
	if(tinf != TINF_OK)
		goto err;

	gcdb_rw = SDL_RWFromConstMem(gcdb_txt, gcdb_txt_len_act);
	if(SDL_GameControllerAddMappingsFromRW(gcdb_rw, SDL_TRUE) == -1)
		goto err;

	SDL_free(gcdb_txt);

	if((in_ctx->input_cmd_event = SDL_RegisterEvents(1)) == (Uint32)-1)
	{
		SDL_LogWarn(SDL_LOG_CATEGORY_INPUT,
			"Special input commands will not be available: %s",
			SDL_GetError());
	}

	/* Set default keyboard keymap. */
	for(unsigned i = 0; i < SDL_arraysize(keymap_defaults); i++)
	{
		/* Set keyboard to JOYPAD input by default. */
		/* TODO: Make smart decisions on default input device based on
		 * what the core supports. */
		input_map(in_ctx, RETRO_INPUT_JOYPAD, keymap_defaults[i].trig,
				  &keymap_defaults[i].map);
	}

	in_ctx->player[0].hai_type = RETRO_INPUT_KEYBOARD;
	in_ctx->player[0].lr_type = in_ctx->player[0].hai_type;

	SDL_LogVerbose(SDL_LOG_CATEGORY_INPUT, "Initialised keyboard input");
	return;

err:
	SDL_free(gcdb_txt);
	SDL_LogWarn(SDL_LOG_CATEGORY_INPUT,
		    "Unable to initialise input system: %s",
		    SDL_GetError());
	return;
}

static SDL_bool input_is_analogue(SDL_GameController *gc)
{
	SDL_GameControllerAxis axis;

	/* FIXME: Have to wait for device remap? */
	/* If there's a single analogue input, set to analogue controller. */
	/* FIXME: Check if it matches an actual dualshock 2. */
	for(axis = 0; axis < SDL_CONTROLLER_AXIS_MAX; axis++)
	{
		SDL_GameControllerButtonBind bind =
			SDL_GameControllerGetBindForAxis(gc, axis);

		if(bind.bindType == SDL_CONTROLLER_BINDTYPE_AXIS)
			return SDL_TRUE;
	}

	return SDL_FALSE;
}

void input_handle_event(struct input_ctx_s *const in_ctx, const SDL_Event *ev)
{
	if(ev->type == SDL_KEYDOWN)
		input_set(in_ctx, ev->key.keysym.scancode, SDL_PRESSED);
	else if(ev->type == SDL_KEYUP)
		input_set(in_ctx, ev->key.keysym.scancode, SDL_RELEASED);
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

		in_ctx->player[0].hai_type = input_is_analogue(gc) ?
				RETRO_INPUT_ANALOG : RETRO_INPUT_JOYPAD;
		in_ctx->player[0].lr_type = in_ctx->player[0].hai_type;
		in_ctx->player[0].gc = gc;

		SDL_GameControllerSetPlayerIndex(gc, 1);

		/* FIXME: assign controller mapping to core. */

		SDL_LogInfo(SDL_LOG_CATEGORY_INPUT,
			    "Controller %s connected as a %s device",
			    gc_name, input_type_str[in_ctx->player[0].hai_type]);
	}
	else if(ev->type == SDL_CONTROLLERDEVICEREMOVED)
	{
		SDL_GameController *gc;
		const char *gc_name;
		const char *const gc_no_name = "with no name";
		gc = SDL_GameControllerFromInstanceID(ev->cdevice.which);
		if(NULL)
			return;

		gc_name = SDL_GameControllerName(gc);
		if(gc_name == NULL)
			gc_name = gc_no_name;

		if(gc == in_ctx->player[0].gc)
		{
			SDL_LogInfo(SDL_LOG_CATEGORY_INPUT,
				    "Player 1 disconnected");
		}

		SDL_GameControllerClose(gc);
		SDL_LogInfo(SDL_LOG_CATEGORY_INPUT,
			    "Controller %s has been disconnected", gc_name);

		/* TODO: Could just zero the struct? */
		in_ctx->player[0].hai_type = RETRO_INPUT_NONE;
		in_ctx->player[0].lr_type = RETRO_INPUT_NONE;
		in_ctx->player[0].gc = NULL;

		/* Change player 1 to keyboard if no other players/controllers
		 * are connected. */
		in_ctx->player[0].hai_type = RETRO_INPUT_KEYBOARD;
		in_ctx->player[0].lr_type = RETRO_INPUT_JOYPAD;
	}
	else if(ev->type == SDL_CONTROLLERDEVICEREMAPPED)
	{
		SDL_LogVerbose(SDL_LOG_CATEGORY_INPUT, "Controller remapped");
	}
}

static void mod_bit(Uint16 *n, Uint8 pos, unsigned val)
{
	unsigned mask = 1 << pos;
	*n = (*n & ~mask) | ((val << pos) & mask);
}

static void input_set(struct input_ctx_s *const in_ctx, SDL_Scancode sc,
		      Uint8 state)
{
	switch(keymap[sc].input_cmd_type)
	{
	case INPUT_CMD_RETRO_INPUT:
		mod_bit(&in_ctx->player[0].retro_state, keymap[sc].input_cmd,
			state);
		break;

	case INPUT_CMD_CALL_FUNC:
		if(in_ctx->input_cmd_event == (Uint32) - 1)
			break;

		if(!state)
			break;

		{
			SDL_Event event = { 0 };
			event.type = in_ctx->input_cmd_event;
			event.user.code = keymap[sc].input_cmd;
			SDL_PushEvent(&event);
		}
		break;
	}
}

Sint16 input_get(const struct input_ctx_s *const in_ctx,
				 unsigned port, unsigned device, unsigned index,
				 unsigned id)
{
	static SDL_GameControllerButton lr_to_gcb[] =
	{
		SDL_CONTROLLER_BUTTON_B,
		SDL_CONTROLLER_BUTTON_Y,
		SDL_CONTROLLER_BUTTON_BACK,
		SDL_CONTROLLER_BUTTON_START,
		SDL_CONTROLLER_BUTTON_DPAD_UP,
		SDL_CONTROLLER_BUTTON_DPAD_DOWN,
		SDL_CONTROLLER_BUTTON_DPAD_LEFT,
		SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
		SDL_CONTROLLER_BUTTON_A,
		SDL_CONTROLLER_BUTTON_X,
		SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
		SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
		-1, /* L2 */
		-1, /* R2 */
		SDL_CONTROLLER_BUTTON_LEFTSTICK,
		SDL_CONTROLLER_BUTTON_RIGHTSTICK
	};

	if(index != 0 || port >= MAX_PLAYERS)
		return 0;

	if(in_ctx->player[port].hai_type != (input_type)device)
	{
		static Uint8 log_lim = 0;
		if(((log_lim >> port) & 0b1) == 0)
		{
			SDL_LogVerbose(SDL_LOG_CATEGORY_INPUT,
				"Core has misidentified device %s on player %u "
				"as %s",
				input_type_str[in_ctx->player[port].hai_type], port,
				input_type_str[device]);
			SDL_LogVerbose(SDL_LOG_CATEGORY_INPUT,
				"This error will no longer appear for player "
				"%d", port);
		}
		log_lim |= 0b1 << port;
		//return 0;
	}

	switch(device)
	{
	case RETRO_INPUT_ANALOG:
		/* Only analogue input devices are supported by libretro analog
		 * inputs. */

		if(index < RETRO_DEVICE_INDEX_ANALOG_BUTTON)
		{
			const SDL_GameControllerAxis lr_to_gcax[2][2] = {
				{ SDL_CONTROLLER_AXIS_LEFTX, SDL_CONTROLLER_AXIS_LEFTY },
				{ SDL_CONTROLLER_AXIS_RIGHTX, SDL_CONTROLLER_AXIS_RIGHTY }
			};

			//SDL_Log("Axis %d: %d", index, SDL_GameControllerGetAxis(in_ctx->player[port].gc, lr_to_gcax[index][id]));

			return SDL_GameControllerGetAxis(in_ctx->player[port].gc,
							 lr_to_gcax[index][id]);
		}
		/* Fall-through */
	case RETRO_INPUT_JOYPAD:
	{
		if(in_ctx->player[port].hai_type == RETRO_INPUT_KEYBOARD)
			return (in_ctx->player[port].retro_state >> id) & 0b1;

		SDL_GameControllerButton btn = lr_to_gcb[id];
		if(btn != -1)
			return SDL_GameControllerGetButton(in_ctx->player[port].gc, btn);
		else if(id == RETRO_DEVICE_ID_JOYPAD_L2)
			return SDL_GameControllerGetAxis(in_ctx->player[port].gc, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
		else if(id == RETRO_DEVICE_ID_JOYPAD_R2)
			return SDL_GameControllerGetAxis(in_ctx->player[port].gc, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);

		break;
	}
	}
	return 0;
}
