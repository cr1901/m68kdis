#!/usr/local/bin/perl

#                 Author:  Christopher G. Phillips
#              Copyright (C) 1994 All Rights Reserved
#
#                              NOTICE
#
# Permission to use, copy, modify, and distribute this software and
# its documentation for any purpose and without fee is hereby granted
# provided that the above copyright notice appear in all copies and
# that both the copyright notice and this permission notice appear in
# supporting documentation.
#
# The author makes no representations about the suitability of this
# software for any purpose.  This software is provided ``as is''
# without express or implied warranty.

while (<>) {
	if (/^Bytes ([0-9a-fA-F]+) to [0-9a-fA-F]+$/) {
		printf("0x%lx\n", hex($1) - 4);
		printf("0x%lx\n", hex($1) - 2);
	}
}
