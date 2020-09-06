#include <load.h>

extern const void *fn_links_snes9x2010[];
extern const void *ext_fn_links_snes9x2010[];
extern const void *fn_links_mini64[];
extern const void *ext_fn_links_mini64[];

const struct links_list_s internal_cores[] = {
	{ fn_links_snes9x2010,	ext_fn_links_snes9x2010	},
	{ fn_links_mini64,	fn_links_mini64		}
};
