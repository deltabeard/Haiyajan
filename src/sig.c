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

#include <sig.h>

static void sig_handler(int sig)
{
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
	      "If your system is configured to do so, a core dump will be "
	      "generated containing information that may help in resolving "
	      "this issue.\n"
	      "If this issue persists, please open a bug report (with your "
	      "core dump file attached if possible), to "
	      "https://github.com/deltabeard/Haiyajan/issues or email "
	      "bugs@deltabeard.com\n",
	      stderr);

	fputs("Haiyajan will now abort.\n", stderr);
	fflush(stderr);

	signal(SIGABRT, SIG_DFL);
	abort();
}

void init_sig(void)
{
	signal(SIGABRT, sig_handler);
	signal(SIGFPE, sig_handler);
	signal(SIGILL, sig_handler);
	signal(SIGSEGV, sig_handler);
}
