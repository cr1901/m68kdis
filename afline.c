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
 * The functions in this file customize the behavior of m68kdis
 * to take certain word values as acceptable A-line and F-line
 * instructions and whether any of these unconditionally change PC.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "dis.h"

#define F	0
#define A	1

struct afinst {
	word_t	inst;
	char	*name;
};

static int
afsort(const void *v1, const void *v2)
{
	struct afinst	*p1 = (struct afinst *)v1;
	struct afinst	*p2 = (struct afinst *)v2;

	if (p1->inst < p2->inst)
		return -1;
	else if (p1->inst > p2->inst)
		return 1;
	else
		return 0;
}

static size_t	njinsts = 0;
static word_t	*jinsts = NULL;

/*
 * Read through ``jfile'' and collect instruction values in ``jinsts''.
 */
static void
readjfile(void)
{
	FILE	*jfp;
	char	buf[80];
	char	*cp;
	word_t	ul;
	word_t	*jtmp;

	if (jfp = fopen(jfile, "r")) {
		while (fgets(buf, sizeof buf, jfp)) {
			buf[strlen(buf) - 1] = '\0'; /* zap '\n' */
			ul = strtoul(buf, &cp, 16);
			if (cp == buf
			  || !((ul >= 0xa000 && ul <= 0xafff)
			  || (ul >= 0xf000 && ul <= 0xffff))) {
				fprintf(stderr, "%s: File %s: bad inst: %s\n",
				  sfile, jfile, buf);
				continue;
			}
			if ((jtmp = realloc(jinsts,
			  ++njinsts * sizeof(*jinsts))) == NULL) {
				if (jtmp)
					jinsts = jtmp;
				perror("realloc");
				(void)fclose(jfp);
				return;
			} else {
				jinsts = jtmp;
				jinsts[njinsts - 1] = ul;
			}
		}
		(void)fclose(jfp);
	} else
		perror(jfile);
}

/*
 * Test whether ``word'' matches an instruction value in ``jfile''.
 */
static int
itsajinst(word_t word)
{
	size_t	i;

	for (i = 0; i < njinsts; i++)
		if (word == jinsts[i])
			return 1;

	return 0;
}

/*
 * Test whether ``word'' is a valid A- or F-line instruction,
 * reading ``[af]file'' as necessary.
 */
static char *
validafinst(int anotf, word_t word)
{
	static int		fileread[2] = { 0, 0 };
	static size_t		ninsts[2] = { 0, 0 };
	static struct afinst	*afinsts[2] = { NULL, NULL };
	struct afinst		*aftmp;
	size_t			i;
	static int		jfileread = 0;

	if (anotf && !afile || !anotf && !ffile)
		return NULL;

	/*
	 * Read ``jfile'' if necessary.
	 */
	if (jfile && !jfileread) {
		readjfile();
		jfileread = 1;
	}

	/*
	 * Read ``[af]file'' if necessary, collecting the values
	 * in ``[af]insts'' and then sorting them.
	 */
	if (!fileread[anotf]) {
		FILE	*afp;

		fileread[anotf] = 1;
		if (afp = fopen(anotf ? afile : ffile, "r")) {
			char	afbuf[80];
			word_t	ul;
			char	*cp;

			while (fgets(afbuf, sizeof afbuf, afp)) {
				afbuf[strlen(afbuf) - 1] = '\0'; /* zap '\n' */
				ul = strtoul(afbuf, &cp, 16);
				if (cp == afbuf
				  || (anotf && (ul < 0xa000 || ul > 0xafff))
				  || (!anotf && (ul < 0xf000 || ul > 0xffff))) {
					fprintf(stderr,
					  "%s: File %s: bad inst: %s\n", sfile,
					    anotf ? afile : ffile, afbuf);
					continue;
				}

				/*
				 * Find instruction-string.
				 * Use "UNKNOWN" if not given.
				 */
				cp--;
				while (*++cp && isspace(*cp))
					;
				if (!cp)
					cp = "UNKNOWN";

				if ((aftmp = realloc(afinsts[anotf],
				  ++ninsts[anotf] * sizeof(*afinsts[anotf])))
				  == NULL
				  || (aftmp[ninsts[anotf] - 1].name
				  = malloc(strlen(cp) + 1)) == NULL) {
					if (aftmp)
						afinsts[anotf] = aftmp;
					perror("realloc");
					return NULL;
				} else {
					afinsts[anotf] = aftmp;
					afinsts[anotf][ninsts[anotf] - 1].inst
					  = ul;
					strcpy(
					 afinsts[anotf][ninsts[anotf] - 1].name,
					 cp);
				}
			}
			(void)fclose(afp);
			qsort(afinsts[anotf], ninsts[anotf],
			  sizeof(*afinsts[anotf]), afsort);
		} else
			perror(anotf ? afile : ffile);
	}
	for (i = 0; i < ninsts[anotf]; i++)
		if (word == afinsts[anotf][i].inst)
			return afinsts[anotf][i].name;

	return NULL;
}

/*
 * Verify whether ``word'' is an acceptable A-line instruction.
 * Print as necessary.
 */
void
aline(word_t word)
{
	char	*cp;

	if (cp = validafinst(A, word)) {
		instprint(ops2f(0), cp);
		valid = 1;
		if (itsajinst(word))
			flags |= ISJMP;
	}
}

/*
 * Verify whether ``word'' is an acceptable F-line instruction.
 * Print as necessary.
 */
void
fline(word_t word)
{
	char	*cp;

	if (cp = validafinst(F, word)) {
		instprint(ops2f(0), cp);
		valid = 1;
		if (itsajinst(word))
			flags |= ISJMP;
	}
}
