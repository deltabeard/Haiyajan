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

#include <SDL.h>
#include <libretro.h>
#include <input.h>
#include <tinf.h>

struct keymap_info_s
{
	/* A command type listed in input_cmd_type. */
	unsigned char cmd_type : 2;

	/* The command to execute. Either input_cmd_event or
	 * input_cmd_input. */
	unsigned char cmd : 6;
};

static const char *const input_type_str[] = {
	"None", "Joypad", "Mouse", "Keyboard", "Lightgun", "Analogue", "Pointer"
};

static struct keymap_info_s keymap[512] = { 0 };

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
	static const struct
	{
		SDL_Scancode sc;
		struct keymap_info_s map;
	} keymap_defaults[] =
	{
		{ SDL_SCANCODE_Z,	{ INPUT_CMD_INPUT, INPUT_JOYPAD_B }},
		{ SDL_SCANCODE_A,	{INPUT_CMD_INPUT, INPUT_JOYPAD_Y }},
		{ SDL_SCANCODE_BACKSPACE,{ INPUT_CMD_INPUT, INPUT_JOYPAD_SELECT }},
		{ SDL_SCANCODE_RETURN,	{ INPUT_CMD_INPUT, INPUT_JOYPAD_START }},
		{ SDL_SCANCODE_UP,	{ INPUT_CMD_INPUT, INPUT_JOYPAD_UP }},
		{ SDL_SCANCODE_DOWN,	{ INPUT_CMD_INPUT, INPUT_JOYPAD_DOWN }},
		{ SDL_SCANCODE_LEFT,	{ INPUT_CMD_INPUT, INPUT_JOYPAD_LEFT }},
		{ SDL_SCANCODE_RIGHT,	{ INPUT_CMD_INPUT, INPUT_JOYPAD_RIGHT }},
		{ SDL_SCANCODE_X,	{ INPUT_CMD_INPUT, INPUT_JOYPAD_A }},
		{ SDL_SCANCODE_S,	{ INPUT_CMD_INPUT, INPUT_JOYPAD_X }},
		{ SDL_SCANCODE_Q,	{ INPUT_CMD_INPUT, INPUT_JOYPAD_L }},
		{ SDL_SCANCODE_W,	{ INPUT_CMD_INPUT, INPUT_JOYPAD_R }},
		{ SDL_SCANCODE_1,	{ INPUT_CMD_INPUT, INPUT_JOYPAD_L2 }},
		{ SDL_SCANCODE_2,	{ INPUT_CMD_INPUT, INPUT_JOYPAD_R2 }},
		{ SDL_SCANCODE_E,	{ INPUT_CMD_INPUT, INPUT_JOYPAD_L3 }},
		{ SDL_SCANCODE_R,	{ INPUT_CMD_INPUT, INPUT_JOYPAD_R3 }},

		{ SDL_SCANCODE_LEFTBRACKET, { INPUT_CMD_INPUT, INPUT_ANALOGUE_LEFT_Y_NEG }},
		{ SDL_SCANCODE_APOSTROPHE, { INPUT_CMD_INPUT, INPUT_ANALOGUE_LEFT_Y_POS }},
		{ SDL_SCANCODE_BACKSLASH, { INPUT_CMD_INPUT, INPUT_ANALOGUE_LEFT_X_POS }},
		{ SDL_SCANCODE_SEMICOLON, { INPUT_CMD_INPUT, INPUT_ANALOGUE_LEFT_X_NEG }},
		{ SDL_SCANCODE_HOME,	{ INPUT_CMD_INPUT, INPUT_ANALOGUE_RIGHT_Y_NEG }},
		{ SDL_SCANCODE_END,	{ INPUT_CMD_INPUT, INPUT_ANALOGUE_RIGHT_Y_POS }},
		{ SDL_SCANCODE_PAGEDOWN, { INPUT_CMD_INPUT, INPUT_ANALOGUE_RIGHT_X_POS }},
		{ SDL_SCANCODE_DELETE,	{ INPUT_CMD_INPUT, INPUT_ANALOGUE_RIGHT_X_NEG }},

		{ SDL_SCANCODE_I,	{ INPUT_CMD_EVENT, INPUT_EVENT_TOGGLE_INFO }},
		{ SDL_SCANCODE_F,	{ INPUT_CMD_EVENT, INPUT_EVENT_TOGGLE_FULLSCREEN }},
		{ SDL_SCANCODE_P,	{ INPUT_CMD_EVENT, INPUT_EVENT_TAKE_SCREENSHOT }},
		{ SDL_SCANCODE_V,	{ INPUT_CMD_EVENT, INPUT_EVENT_RECORD_VIDEO_TOGGLE }}
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

	in_ctx->player[0].hai_type = RETRO_INPUT_KEYBOARD;

	for(unsigned i = 0; i < SDL_arraysize(keymap_defaults); i++)
		keymap[keymap_defaults[i].sc] = keymap_defaults[i].map;

	SDL_LogVerbose(SDL_LOG_CATEGORY_INPUT, "Initialised keyboard input");
	return;

err:
	SDL_free(gcdb_txt);
	SDL_LogWarn(SDL_LOG_CATEGORY_INPUT,
		    "Unable to initialise input system: %s", SDL_GetError());
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

/* TODO: Change this if-statement mess to use address and pointers. */
static void input_set_keyboard(struct input_device_s *dev, const SDL_Scancode sc,
                      const Uint8 state, const Uint32 input_cmd_event)
{

	if(keymap[sc].cmd_type == INPUT_CMD_NONE)
		return;
	else if(keymap[sc].cmd_type == INPUT_CMD_INPUT &&
	                keymap[sc].cmd <= INPUT_JOYPAD_R3)
	{
		dev->keyboard.btns.btns = (dev->keyboard.btns.btns & ~(1 << keymap[sc].cmd)) | (state << keymap[sc].cmd);
	}
	else if(keymap[sc].cmd_type == INPUT_CMD_INPUT)
	{
		switch(keymap[sc].cmd)
		{
		case INPUT_ANALOGUE_LEFT_X_POS:
			dev->keyboard.left_x = state ? INT16_MAX : 0;
			break;
		case INPUT_ANALOGUE_LEFT_X_NEG:
			dev->keyboard.left_x = state ? INT16_MIN : 0;
			break;
		case INPUT_ANALOGUE_LEFT_Y_POS:
			dev->keyboard.left_y = state ? INT16_MAX : 0;
			break;
		case INPUT_ANALOGUE_LEFT_Y_NEG:
			dev->keyboard.left_y = state ? INT16_MIN : 0;
			break;
		case INPUT_ANALOGUE_RIGHT_X_POS:
			dev->keyboard.right_x = state ? INT16_MAX : 0;
			break;
		case INPUT_ANALOGUE_RIGHT_X_NEG:
			dev->keyboard.right_x = state ? INT16_MIN : 0;
			break;
		case INPUT_ANALOGUE_RIGHT_Y_POS:
			dev->keyboard.right_y = state ? INT16_MAX : 0;
			break;
		case INPUT_ANALOGUE_RIGHT_Y_NEG:
			dev->keyboard.right_y = state ? INT16_MIN : 0;
			break;
			/* FIXME: INPUT_ANALOGUE_BTN ? */
		}
	}
	else if(keymap[sc].cmd_type == INPUT_CMD_EVENT && state != 0 &&
	                input_cmd_event != ((Uint32) - 1))
	{
		SDL_Event event;
		event.type = input_cmd_event;
		event.user.code = keymap[sc].cmd;
		SDL_PushEvent(&event);
	}
}

void input_handle_event(struct input_ctx_s *const in_ctx, const SDL_Event *ev)
{
	if(ev->type == SDL_KEYDOWN && in_ctx->player[0].hai_type == RETRO_INPUT_KEYBOARD)
	{
		input_set_keyboard(&in_ctx->player[0], ev->key.keysym.scancode,
			  SDL_PRESSED, in_ctx->input_cmd_event);
	}
	else if(ev->type == SDL_KEYUP && in_ctx->player[0].hai_type == RETRO_INPUT_KEYBOARD)
	{
		input_set_keyboard(&in_ctx->player[0], ev->key.keysym.scancode,
			  SDL_RELEASED, in_ctx->input_cmd_event);
	}
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

		in_ctx->player[0].pad.ctx = gc;
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

		if(gc == in_ctx->player[0].pad.ctx)
		{
			SDL_LogInfo(SDL_LOG_CATEGORY_INPUT,
				    "Player 1 disconnected");
		}

		SDL_GameControllerClose(gc);
		SDL_LogInfo(SDL_LOG_CATEGORY_INPUT,
			    "Controller %s has been disconnected", gc_name);

		/* TODO: Could just zero the struct? */
		in_ctx->player[0].hai_type = RETRO_INPUT_NONE;
		in_ctx->player[0].pad.ctx = NULL;

		/* Change player 1 to keyboard if no other players/controllers
		 * are connected. */
		in_ctx->player[0].hai_type = RETRO_INPUT_KEYBOARD;
	}
	else if(ev->type == SDL_CONTROLLERDEVICEREMAPPED)
	{
		SDL_LogVerbose(SDL_LOG_CATEGORY_INPUT, "Controller remapped");
	}
}

void input_add_controller(struct input_ctx_s *ctx, unsigned port,
		input_type_e device)
{
	if(port >= MAX_PLAYERS)
		return;

	/* No support for device subclass. */
	if(device >= RETRO_INPUT_MAX)
		return;

	ctx->player[port].available_types |= (1 << device);
	return;
}

Sint16 input_get(const struct input_ctx_s *const in_ctx,
				 unsigned port, unsigned device, unsigned index,
				 unsigned id)
{
#if 1
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
	const SDL_GameControllerAxis lr_to_gcax[2][2] = {
		{ SDL_CONTROLLER_AXIS_LEFTX, SDL_CONTROLLER_AXIS_LEFTY },
		{ SDL_CONTROLLER_AXIS_RIGHTX, SDL_CONTROLLER_AXIS_RIGHTY }
	};
#endif

	if(port >= MAX_PLAYERS)
		return 0;

	if(in_ctx->player[port].hai_type != (input_type_e)device)
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
	}

	switch(in_ctx->player[port].hai_type)
	{
	case RETRO_INPUT_KEYBOARD:
		switch(device)
		{
		case RETRO_INPUT_ANALOG:
		{
			if(id == RETRO_DEVICE_ID_ANALOG_X)
			{
				if(index == RETRO_DEVICE_INDEX_ANALOG_LEFT)
					return in_ctx->player[port].keyboard.left_x;
				else if(index == RETRO_DEVICE_INDEX_ANALOG_RIGHT)
					return in_ctx->player[port].keyboard.right_x;
			}
			else if(id == RETRO_DEVICE_ID_ANALOG_Y)
			{
				if(index == RETRO_DEVICE_INDEX_ANALOG_LEFT)
					return in_ctx->player[port].keyboard.left_y;
				else if(index == RETRO_DEVICE_INDEX_ANALOG_RIGHT)
					return in_ctx->player[port].keyboard.right_y;
			}

			return 0;
		}
		case RETRO_INPUT_JOYPAD:
			return (in_ctx->player[port].keyboard.btns.btns >> id) & 1;
		}
		break;

	case RETRO_INPUT_JOYPAD:
	case RETRO_INPUT_ANALOG:
	{
		switch(device)
		{

		case RETRO_INPUT_ANALOG:
			/* Only analogue input devices are supported by libretro analog
			* inputs. */

			if(index < RETRO_DEVICE_INDEX_ANALOG_BUTTON)
			{
				return SDL_GameControllerGetAxis(in_ctx->player[port].pad.ctx,
				                                 lr_to_gcax[index][id]);
			}
		/* Fall-through */
		case RETRO_INPUT_JOYPAD:
		{
			SDL_GameControllerButton btn = lr_to_gcb[id];
			if(btn != -1)
				return SDL_GameControllerGetButton(in_ctx->player[port].pad.ctx, btn);
			else if(id == RETRO_DEVICE_ID_JOYPAD_L2)
				return SDL_GameControllerGetAxis(in_ctx->player[port].pad.ctx, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
			else if(id == RETRO_DEVICE_ID_JOYPAD_R2)
				return SDL_GameControllerGetAxis(in_ctx->player[port].pad.ctx, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);

			break;
		}
		}
	}
	default:
		break;
	}

	return 0;
}
