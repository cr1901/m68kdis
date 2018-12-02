/*
 *                 Author:  Christopher G. Phillips
 *              Copyright (C) 1994 All Rights Reserved
 *
 *                              NOTICE
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted
 * provided that the above copyright notice appear in all copies and
 * that both the copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * The author makes no representations about the suitability of this
 * software for any purpose.  This software is provided ``as is''
 * without express or implied warranty.
 */

#include <stdio.h>
#include <stdlib.h>

char
getachar(void)
{
	int	first = 1;
	char	result = 0;
	int	c;

	while ((c = getchar()) != EOF) {
		if (c >= '0' && c <= '9')
			if (first) {
				result = (c - '0') << 4;
				first = 0;
			} else {
				result |= c - '0';
				return result;
			}
		if (c >= 'A' && c <= 'F')
			if (first) {
				result = (c - 'A' + 10) << 4;
				first = 0;
			} else {
				result |= c - 'A' + 10;
				return result;
			}
		if (c >= 'a' && c <= 'f')
			if (first) {
				result = (c - 'a' + 10) << 4;
				first = 0;
			} else {
				result |= c - 'a' + 10;
				return result;
			}
	}

	exit(0);
}

int
main(int argc, char **argv)
{
	char	c;

	while (1) {
		c = getachar();
		putchar(c);
		fflush(stdout);
	}
}
