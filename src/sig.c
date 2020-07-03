#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

void almost_c99_signal_handler(int sig)
{
	switch(sig)
	{
		case SIGABRT:
			fputs("Caught SIGABRT: usually caused by an abort() or assert()\n", stderr);
			break;
		case SIGFPE:
			fputs("Caught SIGFPE: arithmetic exception, such as divide by zero\n", stderr);
			break;
		case SIGILL:
			fputs("Caught SIGILL: illegal instruction\n", stderr);
			break;
		case SIGINT:
			fputs("Caught SIGINT: interactive attention signal, probably a ctrl+c\n", stderr);
			break;
		case SIGSEGV:
			fputs("Caught SIGSEGV: segfault\n", stderr);
			break;
		case SIGTERM:
		default:
			fputs("Caught SIGTERM: a termination request was sent to the program\n", stderr);
			break;
	}

	_Exit(EXIT_FAILURE);
}

void set_signal_handler()
{
	signal(SIGABRT, almost_c99_signal_handler);
	signal(SIGFPE,  almost_c99_signal_handler);
	signal(SIGILL,  almost_c99_signal_handler);
	signal(SIGINT,  almost_c99_signal_handler);
	signal(SIGSEGV, almost_c99_signal_handler);
	signal(SIGTERM, almost_c99_signal_handler);
}
