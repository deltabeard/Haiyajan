#include <libretro.h>

void retro_init(void)
{
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
	info->library_name = "Init";
	info->library_version = "1";
	info->valid_extensions = NULL;
	info->need_fullpath = false;
	info->block_extract = false;
}

static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_environment_t environ_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

void retro_get_system_av_info(struct retro_system_av_info *info)
{
	info->timing = (struct retro_system_timing){
		.fps = 60.0,
		.sample_rate = 44100.0,
	};

	info->geometry = (struct retro_game_geometry){
		.base_width = 320,
		.base_height = 240,
		.max_width = 320,
		.max_height = 240,
		.aspect_ratio = -1.0,
	};
}

void retro_set_environment(retro_environment_t cb)
{
	environ_cb = cb;

	bool no_content = true;
	cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_content);
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
	environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, NULL);
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
