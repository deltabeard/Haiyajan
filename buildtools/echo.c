/**
 * Tiny non-standard echo.
 *
 * Copyright (c) 2020 Mahyar Koshkouei
 * Redistribution and use in any form, with or without modification, is licit.
 * THIS SOFTWARE IS PROVIDED 'AS-IS', WITHOUT ANY EXPRESS OR IMPLIED WARRANTY.
 * IN NO EVENT WILL THE AUTHORS BE HELD LIABLE FOR ANY DAMAGES ARISING FROM THE
 * USE OF THIS SOFTWARE.
 */

#include <stdio.h>

int main(int argc, char *argv[])
{
	(void) argc;

	while(*(++argv) != NULL)
	{
		fputs(*argv, stdout);
		putc(' ', stdout);
	}

	putc('\n', stdout);
	return 0;
}
