#include <SDL2/SDL.h>
#include <stdlib.h>

#include <libretro.h>
#include <load.h>

static uint_fast8_t prerun_checks(void)
{
	SDL_version compiled;
	SDL_version linked;

	SDL_VERSION(&compiled);
	SDL_GetVersion(&linked);

	if(compiled.major != linked.major)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_SYSTEM,
				"The major version of SDL2 loaded (%d) does "
				"not match the version from which Parsley was "
				"compiled with (%d). "
				"Please recompile Parsley and try again.",
				linked.major, compiled.major);
		return 1;
	}

	if(SDL_VERSIONNUM(compiled.major, compiled.minor, compiled.patch) !=
	   SDL_VERSIONNUM(linked.major, linked.minor, linked.patch))
	{
		SDL_LogWarn(
			SDL_LOG_CATEGORY_SYSTEM,
			"The version of SDL2 loaded (%d.%d.%d) is different to "
			"the version that Parsley was compiled with "
			"(%d.%d.%d).",
			linked.major, linked.minor, linked.patch,
			compiled.major, compiled.minor, compiled.patch);
	}

	return 0;
}

int main(int argc, char *argv[])
{
	struct core_ctx_s ctx;

	if(SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
				"SDL initialisation failed: %s\n",
				SDL_GetError());
		exit(EXIT_FAILURE);
	}

	if(prerun_checks())
		goto err;

	if(argc != 2)
	{
		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "%s CORE\n", argv[0]);
		goto err;
	}

	if(load_libretro_core(argv[1], &ctx))
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "%s\n",
				SDL_GetError());
		goto err;
	}

	if(ctx.retro_api_version() != load_compiled_retro_api_version())
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
				"The loaded libretro core is not compatible "
				"with Parsley.\n");
		goto err;
	}

	{
		struct retro_system_info info;
		ctx.retro_get_system_info(&info);
		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
			    "Libretro core \"%s\" loaded successfully.\n",
			    info.library_name);
	}

	unload_libretro_core(&ctx);
	SDL_Quit();
	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Exiting gracefully.\n");

	exit(EXIT_SUCCESS);

err:
	SDL_Quit();
	exit(EXIT_FAILURE);
}
