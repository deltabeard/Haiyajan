#include <SDL2/SDL.h>

#include <libretro.h>

static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_environment_t environ_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

const unsigned width = 320;
const unsigned height = 240;

static Uint16 *fb;

static struct av_ctx_s
{
	unsigned remaining_frames;
} av_ctx;

/**
 * Tests frameskip, framerate timing and audio timing.
 */

void retro_init(void)
{
	/**
	 * Frame 1: Blue.
	 * Frame 2: Red. This frame will be skipped by the front end.
	 * Frame 3: Green. Pass test.
	 */
	av_ctx.remaining_frames = 3;

	fb = SDL_calloc(width * height, sizeof(Uint16));
}

void retro_deinit(void)
{
}

unsigned retro_api_version(void)
{
	return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
}

void retro_get_system_info(struct retro_system_info *info)
{
	info->library_name = "Test AV";
	info->library_version = "1";
	info->valid_extensions = NULL;
	info->need_fullpath = false;
	info->block_extract = false;
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
	info->timing = (struct retro_system_timing)
	{
		.fps = 100.0,
		.sample_rate = 48000.0,
	};

	info->geometry = (struct retro_game_geometry)
	{
		.base_width = width,
		.base_height = height,
		.max_width = width,
		.max_height = height,
		.aspect_ratio = -1.0,
	};
}

void retro_set_environment(retro_environment_t cb)
{
	environ_cb = cb;

	bool no_content = true;
	cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_content);

	enum retro_pixel_format pixel_format = RETRO_PIXEL_FORMAT_RGB565;
	SDL_assert_always(
		cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &pixel_format) == true);
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
	audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
	audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
	input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
	input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
	video_cb = cb;
}

void retro_reset(void)
{
}

void retro_run(void)
{
	Uint16 colour;
	Uint16 *p = fb;

	switch(av_ctx.remaining_frames)
	{
	case 3:
		/* Blue. */
		colour = 0b11111;
		break;

	case 2:
		/* Red. */
		colour = 0b1111100000011111;
		break;

	case 1:
		/* Green. */
		colour = 0b0000011111100000;
		break;

	default:
		SDL_assert_always(
			environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, NULL) == true);
		return;
	}

	for(unsigned i = width * height; i > 0; i--)
		*(p++) = colour;

	video_cb(fb, width, height, width * sizeof(Uint16));
}

bool retro_load_game(const struct retro_game_info *info)
{
	return true;
}

void retro_unload_game(void)
{
}

unsigned retro_get_region(void)
{
	return RETRO_REGION_NTSC;
}

size_t retro_serialize_size(void)
{
	return 0;
}

bool retro_serialize(void *data_, size_t size)
{
	return false;
}

bool retro_unserialize(const void *data_, size_t size)
{
	return false;
}

void *retro_get_memory_data(unsigned id)
{
	return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
	return 0;
}

void retro_cheat_reset(void)
{
}

bool retro_load_game_special(unsigned game_type,
	const struct retro_game_info *info,
	size_t num_info)
{
	return false;
}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
	(void)index;
	(void)enabled;
	(void)code;
}
