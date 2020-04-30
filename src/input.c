#include <SDL2/SDL.h>
#include <libretro.h>
#include <haiyajan.h>
#include <input.h>

enum input_type_e {
	INPUT_CMD_RETRO_INPUT,
	INPUT_CMD_CALL_FUNC
};

typedef enum input_type_e input_type;

struct keymap_s
{
	SDL_Keycode sdl_keycode;
	const input_type in_type;

	const union
	{
		unsigned retro_id;
		void (*fn)(struct input_ctx_s *const);
	};
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
	{ SDLK_x,	INPUT_CMD_RETRO_INPUT,	{ RETRO_DEVICE_ID_JOYPAD_B }	},
	{ SDLK_s,	INPUT_CMD_RETRO_INPUT,	{ RETRO_DEVICE_ID_JOYPAD_Y }	},
	{ SDLK_BACKSPACE, INPUT_CMD_RETRO_INPUT, { RETRO_DEVICE_ID_JOYPAD_SELECT}},
	{ SDLK_RETURN,	INPUT_CMD_RETRO_INPUT,	{ RETRO_DEVICE_ID_JOYPAD_START }},
	{ SDLK_UP,	INPUT_CMD_RETRO_INPUT,	{ RETRO_DEVICE_ID_JOYPAD_UP }	},
	{ SDLK_DOWN,	INPUT_CMD_RETRO_INPUT,	{ RETRO_DEVICE_ID_JOYPAD_DOWN }	},
	{ SDLK_LEFT,	INPUT_CMD_RETRO_INPUT,	{ RETRO_DEVICE_ID_JOYPAD_LEFT }	},
	{ SDLK_RIGHT,	INPUT_CMD_RETRO_INPUT,	{ RETRO_DEVICE_ID_JOYPAD_RIGHT }},
	{ SDLK_z,	INPUT_CMD_RETRO_INPUT,	{ RETRO_DEVICE_ID_JOYPAD_A }	},
	{ SDLK_a,	INPUT_CMD_RETRO_INPUT,	{ RETRO_DEVICE_ID_JOYPAD_X }	},
	{ SDLK_q,	INPUT_CMD_RETRO_INPUT,	{ RETRO_DEVICE_ID_JOYPAD_L }	},
	{ SDLK_w,	INPUT_CMD_RETRO_INPUT,	{ RETRO_DEVICE_ID_JOYPAD_R }	},
	{ SDLK_e,	INPUT_CMD_RETRO_INPUT,	{ RETRO_DEVICE_ID_JOYPAD_L2 }	},
	{ SDLK_r,	INPUT_CMD_RETRO_INPUT,	{ RETRO_DEVICE_ID_JOYPAD_R2 }	},
	{ SDLK_t,	INPUT_CMD_RETRO_INPUT,	{ RETRO_DEVICE_ID_JOYPAD_L3 }	},
	{ SDLK_y,	INPUT_CMD_RETRO_INPUT,	{ RETRO_DEVICE_ID_JOYPAD_R3 }	},

	{ SDLK_i,	INPUT_CMD_CALL_FUNC,	{ .fn = input_toggle_ui_info }	},
	{ SDLK_f,	INPUT_CMD_CALL_FUNC,	{ .fn = input_toggle_fullscreen }}
};

int input_init(struct input_ctx_s *in_ctx)
{
	SDL_zerop(in_ctx);
	in_ctx->input_cmd_event = SDL_RegisterEvents(1);
	return in_ctx->input_cmd_event == (Uint32)-1 ? 1 : 0;
}

void input_set(struct input_ctx_s *in_ctx, SDL_KeyCode code, Sint16 state)
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

	return in_ctx->joypad_state[id];
}
