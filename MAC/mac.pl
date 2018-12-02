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
	if (/^(a([0189])[0-9a-fA-F]{2})\s+(.+)\s*$/) {
		$s = $3;
		$inst = $1;
		$inst =~ tr/A-Z/a-z/;
		$c = $2;
		if ($c =~ /0/) {
			$zero{"$inst"} = $s;
		} elsif ($c =~ /1/) {
			$one{"$inst"} = $s;
		} elsif ($c =~ /8/) {
			$eight{"$inst"} = $s;
		} else {
			$nine{"$inst"} = $s;
		}
	}
}

foreach $key (sort byhex (keys %zero)) {
	$key =~ /^a0(..)/ && ($last2 = $1);
	$v = $zero{"$key"};
	if (defined $one{"$key"}) {
		for ($i = 0; $i < 8; $i += 2) {
			print "a$i$last2 $v\n";
		}
		$v = $one{"$key"};
		for ($i = 1; $i < 8; $i += 2) {
			print "a$i$last2 $v\n";
		}
		undef $one{"$key"};
	} else {
		for ($i = 0; $i < 8; $i++) {
			print "a$i$last2 $v\n";
		}
	}
}
foreach $key (sort byhex (keys %one)) {
	$key =~ /^a1(..)/ && ($last2 = $1);
	$v = $one{"$key"};
	for ($i = 1; $i < 8; $i++) {
		print "a$i$last2 $v\n";
	}
}
foreach $key (sort byhex (keys %eight)) {
	$key =~ /^a8(..)/ && ($last2 = $1);
	$v = $eight{"$key"};
	print "a8$last2 $v\n";
	print "aa$last2 $v\n";
	print "ac$last2 $v\n";
	print "ae$last2 $v\n";
}
foreach $key (sort byhex (keys %nine)) {
	$key =~ /^a9(..)/ && ($last2 = $1);
	$v = $nine{"$key"};
	print "a9$last2 $v\n";
	print "ab$last2 $v\n";
	print "ad$last2 $v\n";
	print "af$last2 $v\n";
}
exit(0);

sub byhex {
	hex($a) - hex($b);
}
