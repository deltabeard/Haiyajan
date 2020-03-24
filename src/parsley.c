#include <stdlib.h>

#include <libretro.h>
#include <load.h>

int main(int argc, char *argv[])
{
	struct libretro_fn_s fn;

	if(argc != 2)
		return EXIT_FAILURE;

	initialise_libretro_core(argv[1], &fn);

	return EXIT_SUCCESS;
}
