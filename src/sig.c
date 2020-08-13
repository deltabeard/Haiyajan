/**
 * Catches error signals.
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

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <haiyajan.h>
#include <sig.h>

static const struct haiyajan_ctx_s *ctx;

static void log_out(void *ctx, int cat, SDL_LogPriority pri, const char *msg)
{
	(void) ctx;
	(void) cat;
	(void) pri;
	fputs(msg, stderr);
	putc('\n', stderr);
}


static void sig_handler(int sig)
{
	SDL_LogSetOutputFunction(log_out, NULL);

	fputs("\nUnfortunately a critical error has occurred due to ", stderr);

	switch(sig)
	{
	case SIGABRT:
		fputs("the calling of an abort() or assert() function (SIGABRT)",
		      stderr);
		break;
	case SIGFPE:
		fputs("an invalid floating point calculation (SIGFPE)", stderr);
		break;
	case SIGILL:
		fputs("the execution of an illegal instruction (SIGILL)",
		      stderr);
		break;
	case SIGSEGV:
		fputs("a segmentation fault (SIGSEGV)", stderr);
		break;
	default:
		fputs("an unknown error", stderr);
		break;
	}

	fputs(".\n"
	      "Haiyajan will print out its context at the time of the error.\n"
	      "If your system is configured to do so, a core dump will also be "
	      "generated.\n"
	      "Both of these items may help in resolving this issue.\n"
	      "If this issue persists, please open a bug report with the "
	      "context trace and core dump (if available) attached at "
	      "https://github.com/deltabeard/Haiyajan/issues or email "
	      "bugs@deltabeard.com\n",
	      stderr);

	/* Just in case SDL_Log() fails. */
	fflush(stderr);

	{
#define fn_max (sizeof(ctx->core.fn)/sizeof(uintptr_t))
		/* Checks if functions are initialised or not. Each function is
		 * represented by a bit in are_fn_set. */
		Uint32 are_fn_set = 0;
		uintptr_t fns[fn_max];
		unsigned fn;

		SDL_memcpy(fns, &ctx->core.fn, sizeof(fns));

		for(fn = 0; fn < fn_max; fn++)
		{
			Uint32 set = fns[fn] != 0 ? 1 : 0;
			are_fn_set |= (set << fn);
		}

		SDL_Log("Haiyajan context trace:\n"
			"STATUS: 0x%02X%s\n"
			"FN: 0x%08X\n", ctx->core.env.status.all,
			ctx->core.env.status.bits.playing == 1 ? " retro_run()" : "",
			are_fn_set);

		if(ctx->core.env.status.bits.core_init == 1)
		{
			SDL_Log("CORE: %s %s\n", ctx->core.sys_info.library_name,
				ctx->core.sys_info.library_version);
		}
#undef fn_max
	}

	fputs("\nHaiyajan will now abort.\n", stderr);
	fflush(stderr);

	signal(SIGABRT, SIG_DFL);
	abort();
}

void init_sig(struct haiyajan_ctx_s *c)
{
	ctx = c;

	signal(SIGABRT, sig_handler);
	signal(SIGFPE, sig_handler);
	signal(SIGILL, sig_handler);
	signal(SIGSEGV, sig_handler);
}
