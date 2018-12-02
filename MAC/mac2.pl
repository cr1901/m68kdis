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
	chop;
	if (/^(a[0-9a-f]{3})\s+(\S.*)$/i) {
		$s = $2;
		$s =~ s/\s+$//;
		$t = $1;
		$t =~ tr/A-Z/a-z/;
		$name{"$t"} = $s;
	}
}

while (<STDIN>) {
	if (/^(a[0-9a-f]{3})\s+/i) {
		$trap = $1;
		$trap =~ tr/A-Z/a-z/;
		if (defined $name{"$trap"}) {
			$v = $name{"$trap"};
			print "$trap $v\n";
			undef $v;
			next;
		}
	}
	print;
}

foreach $key (sort byhex (keys %name)) {
	$v = $name{"$key"};
	print "$key $v\n";
}

sub byhex {
	hex($a) - hex($b);
}
