#include <SDL2/SDL.h>
#include <stdlib.h>

#include <libretro.h>
#include <load.h>

int main(int argc, char *argv[])
{
	struct libretro_fn_s fn;

	if(argc != 2)
	{
		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "%s CORE\n", argv[0]);
		return EXIT_FAILURE;
	}

	if(load_libretro_core(argv[1], &fn))
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "%s\n",
				SDL_GetError());
		return EXIT_FAILURE;
	}

	{
		struct retro_system_info info;
		fn.retro_get_system_info(&info);
		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
			    "Libretro core \"%s\" loaded successfully.\n",
			    info.library_name);
	}

	unload_libretro_core(&fn);
	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Exiting gracefully.\n");

	return EXIT_SUCCESS;
}
