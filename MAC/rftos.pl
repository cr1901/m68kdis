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

if ($#ARGV == 0) {
	$file = $ARGV[0];
	open(INFILE, "<$file.rf.s") || die "$file.rf.s: $!\n";
	die if system("mv", "${file}.rf.s", "$file.rf.s.$$");
	open(OUTFILE, ">$file.rf.s") || die "$file.rf.s: $!\n";
	open(RF, "<$file.out") || die "$file.out: $!\n";
	$n = 0;
	while (<RF>) {
		if (/^Resource type: "(....)"$/) {
			$type = $1;
		} elsif (/^ID (\d+)$/) {
			$id = $1;
		} elsif (/^Bytes ([0-9a-fA-F]+) to ([0-9a-fA-F]+)$/) {
			$start = sprintf("%08lx", hex($1) - 4);
			$assoc{"$start"} = sprintf("%4.4s %d", $type, $id);
			$ends{"$start"} = sprintf("%08lx", hex($2) + 1);
		}
	}
	close(RF);

	@keys = sort keys %assoc;
	if ($#keys == -1) {
		$n = -1;
	} else {
		$n = 0;
		$key = $keys[$n];
		$value = $assoc{"$key"};
	}
	$inresource = 0;
	$newline = 0;
	select OUTFILE;
	while (<INFILE>) {
		if ($inresource) {
			$doit = 0;
			if (/^$end/) {
				$doit = 1;
			} else {
				if (hex($_) > $k) {
					$doit = 1;
		warn "$assoc{$keys[$n - 1]} should not include $end\n";
				}
			}
			if ($doit) {
				print "\n";
				$newline = 1;
				$inresource = 0;
			}
		}
		if (!$inresource && $n >= 0 && /^$key/) {
			if (!$newline && hex($key) > 0) {
				print "\n";
			}
			print "$value\n";
			$end = $ends{"$key"};
			$k = hex($end);
			if (++$n > $#keys) {
				$n = -1;
			} else {
				$key = $keys[$n];
				$value = $assoc{"$key"};
			}
			$inresource = 1;
		}
		$newline = 0;
		print;
	}
	if ($inresource) {
		warn "$value starts at $key but should not include $end\n";
	}
	close(INFILE);
	close(OUTFILE);
	unlink("$file.rf.s.$$");
}
