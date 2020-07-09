#include <SDL.h>

#include <libretro.h>

static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_environment_t environ_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

#define LIBRETRO_WIDTH	1
#define LIBRETRO_HEIGHT	1

static Uint16 fb[LIBRETRO_WIDTH * LIBRETRO_HEIGHT];

/**
 * Tests frameskip, framerate timing and audio timing.
 */

void retro_init(void)
{
	fb[0] = 0x0000;
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
		/* Every 10 frames, a frame must be skipped when the screen is
		 * 10 Hz. */
		.fps = 10,
		.sample_rate = 48000.0,
	};

	info->geometry = (struct retro_game_geometry)
	{
		.base_width = LIBRETRO_WIDTH,
		.base_height = LIBRETRO_HEIGHT,
		.max_width = LIBRETRO_WIDTH,
		.max_height = LIBRETRO_HEIGHT,
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
	fb[0] = ~fb[0];

	video_cb(fb, LIBRETRO_WIDTH, LIBRETRO_HEIGHT,
		LIBRETRO_WIDTH * sizeof(Uint16));
}

bool retro_load_game(const struct retro_game_info *info)
{
	(void) info;
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

bool retro_serialize(void *data, size_t size)
{
	(void) data;
	(void) size;
	return false;
}

bool retro_unserialize(const void *data, size_t size)
{
	(void) data;
	(void) size;
	return false;
}

void *retro_get_memory_data(unsigned id)
{
	(void) id;
	return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
	(void) id;
	return 0;
}

void retro_cheat_reset(void)
{
}

bool retro_load_game_special(unsigned game_type,
	const struct retro_game_info *info,
	size_t num_info)
{
	(void) game_type;
	(void) info;
	(void) num_info;
	return false;
}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
	(void)index;
	(void)enabled;
	(void)code;
}
