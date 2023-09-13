#include <SDL.h>

#include "all.h"
#include "tai.h"

#ifdef _MSC_VER
#define ALIGN(bits) __declspec(align(bits))
#else
#define ALIGN(bits) __attribute__((aligned(bits)))
#endif

#if defined(__WIN32__)
#undef SDL_PRIu64
#define SDL_PRIu64 "I64u"
#endif

ALIGN(8) struct tai_frame_s {
	Uint64 frame_num;
	Uint8 command;
};

ALIGN(8) struct tai_s {
	SDL_RWops *f;
	SDL_bool record;
	Uint64 frame;
	Uint64 next_frame;
};

ALIGN(8) struct tai_header_s {
	Uint8 magic[10];
	Uint8 version;
	Uint8 file_version;
	char rom_target[32];
	char emu_target[32];
	char emu_version[28];
	char tai_desc[32];
	char tai_author[32];
};

#define HTAI_CMD_END		0
#define HTAI_CMD_QUIT		1
#define HTAI_CMD_KEYBOARD	2
#define HTAI_CMD_TEXTEDIT	3
#define HTAI_CMD_CONTROLLER_AXIS	8
#define HTAI_CMD_CONTROLLER_BUTTON	9

ALIGN(8) struct tai_dat_keyboard_s {
	Uint8 state;
	Uint8 unused;
	Uint16 keymod;
	Uint32 scancode;
	Uint32 keycode;
};

tai *tai_init(SDL_RWops *f, SDL_bool record)
{
	tai *ctx = SDL_malloc(sizeof(tai));
	if(ctx == NULL)
		goto out;

	ctx->f = f;
	ctx->record = record;
	ctx->frame = 0;

	if(record)
	{
		size_t ret;
		const struct tai_header_s hdr = {
			.magic = {
				0xAB, 'h', 't', 'a', 'i', 0xBB,
				0x0D, 0x0A, 0x1A, 0x0A
			},
			.version = 1,
			.file_version = 1,
			.rom_target = "Unknown",
			.emu_target = "Unknown",
			.emu_version = "Unknown",
			.tai_desc = "Test",
			.tai_author = "No Author"
		};

		SDL_RWseek(f, 0, RW_SEEK_SET);
		ret = SDL_RWwrite(f, &hdr, 1, sizeof(hdr));
		if(ret < sizeof(hdr))
		{
			SDL_free(ctx);
			ctx = NULL;
			goto out;
		}
	}
	else
	{
		size_t ret;
		struct tai_header_s hdr;
		const Uint8 magic[10] = {
			0xAB, 'h', 't', 'a', 'i', 0xBB, 0x0D, 0x0A, 0x1A, 0x0A
		};

		SDL_RWseek(f, 0, RW_SEEK_SET);
		ret = SDL_RWread(f, &hdr, 1, sizeof(hdr));
		if(ret < sizeof(hdr))
			goto err;

		if(SDL_memcmp(&hdr, magic, sizeof(magic)) != 0)
		{
			SDL_SetError("Invalid tool assisted input file: magic mismatch");
			goto err;
		}

		/* Read first frame information. */
		ret = SDL_RWread(ctx->f, &ctx->next_frame, sizeof(ctx->next_frame), 1);
		if(ret < 1)
			goto err;
	}

	SDL_LogVerbose(SDL_LOG_CATEGORY_TEST, "Initialised TAI module for %s",
			record ? "recording" : "playing");

out:
	return ctx;

err:
	SDL_free(ctx);
	ctx = NULL;
	goto out;
}

void tai_next_frame(tai *ctx)
{
	ctx->frame++;
}

int tai_process_event(tai *ctx, SDL_Event *e)
{
	if(ctx == NULL)
		return 0;

	if(ctx->record == SDL_TRUE)
	{
		if(e == NULL)
			goto out;

		switch(e->type)
		{
#if 0
		case SDL_CONTROLLERBUTTONUP:
		case SDL_CONTROLLERBUTTONDOWN:
			SDL_WriteLE64(ctx->f, ctx->frame);
			SDL_WriteU8(ctx->f, HTAI_CMD_CONTROLLER_BUTTON);
			SDL_WriteU8(ctx->f, e->cbutton.button);
			SDL_WriteU8(ctx->f, e->cbutton.state);
			SDL_RWseek(ctx->f, sizeof(Uint16), RW_SEEK_CUR);
			SDL_LogVerbose(SDL_LOG_CATEGORY_TEST,
				"TAI: Recorded controller button %d at frame %lu",
				e->cbutton.button, ctx->frame);
			break;

		case SDL_CONTROLLERAXISMOTION:
			SDL_RWwrite(ctx->f, &ctx->frame, sizeof(ctx->frame), 1);
			htai_type = HTAI_CMD_CONTROLLER_AXIS;
			SDL_RWwrite(ctx->f, &htai_type, sizeof(htai_type), 1);
			SDL_RWwrite(ctx->f, &e->caxis.axis,
					sizeof(e->caxis.axis), 1);
			SDL_RWseek(ctx->f, sizeof(Uint8), RW_SEEK_CUR);
			SDL_RWwrite(ctx->f, &e->caxis.value,
					sizeof(e->caxis.value), 1);
			SDL_LogVerbose(SDL_LOG_CATEGORY_TEST,
				"TAI: Recorded controller axis modification at frame %lu",
				ctx->frame);
			break;
#endif
		case SDL_QUIT:
			SDL_WriteLE64(ctx->f, ctx->frame);
			SDL_WriteU8(ctx->f, HTAI_CMD_QUIT);
			break;


		case SDL_KEYUP:
		case SDL_KEYDOWN:
			SDL_WriteLE64(ctx->f, ctx->frame);
			SDL_WriteU8(ctx->f, HTAI_CMD_KEYBOARD);
			SDL_WriteU8(ctx->f, e->key.state);
			SDL_WriteU8(ctx->f, 0); /* skip byte */
			SDL_WriteLE16(ctx->f, e->key.keysym.mod);
			SDL_WriteLE32(ctx->f, e->key.keysym.scancode);
			SDL_WriteLE32(ctx->f, e->key.keysym.sym);

			/* TODO: make logging universal */
			SDL_LogVerbose(SDL_LOG_CATEGORY_TEST,
				"TAI: Recorded keyboard input %s %s at frame %" SDL_PRIu64,
				SDL_GetKeyName(e->key.keysym.sym),
				e->key.state ? "pressed" : "released",
				ctx->frame);
			break;

		default:
			break;
		}

		goto out;
	}

	/* If playing tool assisted input file. */
	while(ctx->frame == ctx->next_frame)
	{
		SDL_Event gen;
		size_t ret;
		Uint8 cmd;

		cmd = SDL_ReadU8(ctx->f);

		switch(cmd)
		{
		case HTAI_CMD_END:
			tai_exit(ctx);
			return 0;

		case HTAI_CMD_QUIT:
			gen.type = SDL_QUIT;
			gen.quit.timestamp = SDL_GetTicks();
			SDL_PushEvent(&gen);
			break;

		case HTAI_CMD_KEYBOARD:
		{
			struct tai_dat_keyboard_s keydat;
			ret = SDL_RWread(ctx->f, &keydat, 1, sizeof(keydat));
			if(ret < sizeof(keydat))
			{
				SDL_SetError("Tool assisted input: "
					"Could not read keyboard "
					"command data");
				goto err;
			}

			gen.type = keydat.state ? SDL_KEYUP : SDL_KEYDOWN;
			gen.key.timestamp = SDL_GetTicks(); /* TODO */
			gen.key.state = keydat.state;
			gen.key.repeat = 0;
			gen.key.keysym.mod = keydat.keymod;
			gen.key.keysym.scancode = keydat.scancode;
			gen.key.keysym.sym = keydat.keycode;
			if(SDL_PushEvent(&gen) != 1)
			{
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
					"TAI: Unable to push event: %s",
					SDL_GetError());
				break;
			}
			SDL_LogVerbose(SDL_LOG_CATEGORY_TEST,
				"TAI: Command Keyboard input %s %s at frame %" SDL_PRIu64,
				SDL_GetKeyName(keydat.keycode),
				keydat.state == SDL_PRESSED ? "pressed" : "released",
				ctx->frame);
			break;
		}

		default:
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
				"TAI: Invalid command %hu read at %" SDL_PRIs64 "; exiting.",
				cmd, SDL_RWtell(ctx->f));
			tai_exit(ctx);
			goto err;
		}

		SDL_PumpEvents();

		ret = SDL_RWread(ctx->f, &ctx->next_frame, sizeof(ctx->next_frame), 1);
		if(ret < 1)
			goto err;
	}

out:
	return 0;

err:
	return -1;
}

int tai_exit(tai *ctx)
{
	if(ctx == NULL)
		return 0;

	if(ctx->record == SDL_TRUE)
	{
		SDL_WriteLE64(ctx->f, ctx->frame);
		SDL_WriteU8(ctx->f, HTAI_CMD_END);
	}

	SDL_RWclose(ctx->f);
	SDL_free(ctx);
	ctx = NULL;
	return 0;
}
