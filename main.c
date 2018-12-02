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

/*
 * m68kdis' main (which processes command-line arguments) is here,
 * along with definitions for most global variables.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include "dis.h"

static char	*patchlevel = "m68kdis 1.0";

char	*cc[] = {
	"T", "F", "HI", "LS", "CC", "CS", "NE", "EQ",
	"VC", "VS", "PL", "MI", "GE", "LT", "GT", "LE"
};
char	*bitd[] = {
	"TST", "CHG", "CLR", "SET"
};
char	*bitf[] = {
	"EXTU", "EXTS", "FFO", "INS"
};

/*
 * Buffers for printing instructions
 */
char	buf1[100];
char	buf2[100];
char	buf3[100];

addr_t	curlabel = 0;
int	pass;
int	valid;

FILE	*infp;
FILE	*outfp;
addr_t	pc = 0;
addr_t	ppc = 0;
addr_t	initialpc = 0;
int	chip = 0;
int	lower = 0;
int	minlen = 5;
int	onepass = 0;
int	sp = 0;
int	odd = 0;
int	linkfallthrough = 0;
size_t	slenprint = 30;
#ifndef NOBAD
int	dobad = 0;
#endif
char	*afile = NULL;
char	*bfile = NULL;
char	*ffile = NULL;
char	*ifile = NULL;
char	*jfile = NULL;
char	*nfile = NULL;
char	*nsfile = NULL;

jmp_buf	jmp;
char	*sfile;

static char	*progname;

static void
usage(void)
{
	fprintf(stderr,
"Usage: %s [-00|-08|-10|-20|-30] [-a A-line-file] [-all[c]]\n", progname);
	fprintf(stderr,
"[-b break-file] [-f F-line-file] [-i instruction-file] [-j jump-file] [-l]\n");
	fprintf(stderr,
"[-lft] [-n data-file] [-ns notstart-file] [-o output-file] [-odd]\n");
	fprintf(stderr,
"[-pc initialpc] [-s minlength] [-slenp maxlength] file...\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	int	status = 0;
	char	*ofile = NULL;

	progname = argv[0];

	while (--argc && **++argv == '-') {
		if (strcmp("-pc", *argv) == 0 && argc--) {
			initialpc = strtoul(*++argv, NULL, 0);
		} else if (strcmp("-000", *argv) == 0) {
			chip |= MC68000;
		} else if (strcmp("-008", *argv) == 0) {
			chip |= MC68008;
		} else if (strcmp("-010", *argv) == 0) {
			chip |= MC68010;
		} else if (strcmp("-020", *argv) == 0) {
			chip |= MC68020;
		} else if (strcmp("-030", *argv) == 0) {
			chip |= MC68030;
		} else if (strcmp("-040", *argv) == 0) {
			chip |= MC68040;
		} else if (strcmp("-851", *argv) == 0) {
			chip |= MC68851;
		} else if (strcmp("-881", *argv) == 0) {
			chip |= MC68881;
		} else if (strcmp("-882", *argv) == 0) {
			chip |= MC68882;
		} else if (strcmp("-o", *argv) == 0 && argc--) {
			/*
			 * output pathname
			 */
			ofile = *++argv;
		} else if (strcmp("-i", *argv) == 0 && argc--) {
			/*
			 * file containing offsets
			 * that *are* instructions
			 */
			ifile = *++argv;
		} else if (strcmp("-a", *argv) == 0 && argc--) {
			/*
			 * file containing valid A-line (1010) instructions
			 */
			afile = *++argv;
		} else if (strcmp("-b", *argv) == 0 && argc--) {
			/*
			 * file containing offsets of data for which
			 * a new line of output should be started
			 */
			bfile = *++argv;
		} else if (strcmp("-f", *argv) == 0 && argc--) {
			/*
			 * file containing valid F-line (1111) instructions
			 */
			ffile = *++argv;
		} else if (strcmp("-j", *argv) == 0 && argc--) {
			/*
			 * file containing A-line and F-line
			 * instructions that cause PC to be changed such
			 * that it is not necessary for the next word to
			 * be an instruction
			 */
			jfile = *++argv;
		} else if (strcmp("-n", *argv) == 0 && argc--) {
			/*
			 * file containing offsets that are *not* instructions
			 */
			nfile = *++argv;
		} else if (strcmp("-ns", *argv) == 0 && argc--) {
			/*
			 * file containing offsets that
			 * are not the *start* of instructions
			 */
			nsfile = *++argv;
		} else if (strcmp("-all", *argv) == 0)
			onepass = INCONSISTENT;
		else if (strcmp("-allc", *argv) == 0)
			onepass = CONSISTENT;
		else if (strcmp("-lft", *argv) == 0)
			linkfallthrough = 1;
		else if (strcmp("-odd", *argv) == 0)
			odd = 1;
		else if (strcmp("-sp", *argv) == 0)
			sp = 1;
		else if (strcmp("-l", *argv) == 0) {
			lower = 1;
#ifdef DEBUG
		} else if (strncmp("-d", *argv, 2) == 0) {
			extern int	debug;

			if (isdigit(argv[0][2]))
				debug = atoi(&argv[0][2]);
#endif
#ifndef NOBAD
		} else if (strcmp("-bad", *argv) == 0) {
			dobad = 1;
#endif
		} else if (strcmp("-s", *argv) == 0 && argc--) {
			minlen = atoi(*++argv);
			if (minlen < 2)
				minlen = 2;
		} else if (strcmp("-slenp", *argv) == 0 && argc--) {
			slenprint = atoi(*++argv);
			if (slenprint < 10)
				slenprint = 10;
		} else {
			fprintf(stderr, "%s: bad option: %s\n", progname,
			  *argv);
			usage();
		}
	}

	if (!odd && initialpc & 1) {
		fprintf(stderr, "%s: initialpc odd but -odd not specified\n",
		  progname);
		exit(1);
	}

	if (!CPU(chip))
		chip |= MC68000;

	if (PMMU(chip) && CPU(chip) < MC68020) {
		fprintf(stderr, "%s: bad cpu/coprocessor combination\n",
		  progname);
		exit(1);
	}

	if (argc == 0 && onepass) {
		infp = stdin;
		sfile = "stdin";
		if (setjmp(jmp) == 0)
			disassemble();
	} else if (argc == 1 || argc > 1 && !ofile) {
		argv--;
		while (argc--) {
			char	*lastslash;
			size_t	len;
			size_t	extra;

			if ((infp = fopen(*++argv, "rb")) == NULL) {
				perror(*argv);
				status++;
				continue;
			}

			/*
			 * Determine output filename.
			 * If unspecified, add ".s" to end of input filename.
			 */
			if (ofile)
				sfile = ofile;
			else {
				if (lastslash = strrchr(*argv, '/'))
					*argv = lastslash + 1;
				len = strlen(*argv);
				extra = (len > 2 && argv[0][len - 2] == '.'
				  && argv[0][len - 1] == 'o') ? 0 : 2;
				if ((sfile = malloc(len + extra + 1)) == NULL) {
					perror(*argv);
					status++;
					(void)fclose(infp);
					continue;
				}
				strcpy(sfile, *argv);
				strcpy(&sfile[len - 2 + extra], ".s");
			}
			if ((outfp = fopen(sfile, "w")) == NULL) {
				perror(sfile);
				status++;
				(void)fclose(infp);
				continue;
			}
			if (setjmp(jmp) == 0)
				disassemble();
			(void)fclose(infp);
			(void)fclose(outfp);
			if (!ofile)
				free(sfile);
		}
	} else
		usage();

	exit(status);
}
