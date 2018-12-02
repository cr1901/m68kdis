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
#include <string.h>
#include <ctype.h>
#include "dis.h"

#ifdef DEBUG
#define INITIAL		0x01
#define DELETIONS	0x02
#define TRY		0x04
#define LASTTRY		0x08
#define MAKEGOOD	0x10
#define OVERLAPS	0x20
#define LABELS		0x40

int	debug = 0;
#endif

long	curoffset;	/* current offset into file */
short	flags;		/* flags for current instruction */
addr_t	required[3];	/* instructions that must be valid for current
			   instruction to also be valid */
int	pcrelative = 0;	/* used to signal that PC-relative addresses
			   referenced by JMP and JSR instructions
			   should be stored in ``required'' */

extern char	instbuf[];	/* used to store the nibbles of an
				   instruction in hexadecimal */
extern size_t	leninstbuf;	/* current length of string in instbuf */

struct inst	*insts;		/* instruction database */
addr_t		maxoffset;	/* last offset that may hold an instruction */

/*
 * These macros are used for convenience.  They do the following:
 *
 * NREQD(o)	returns the number of instructions instruction ``o'' depends on
 * JUMP(o)	returns whether instruction ``o'' changes PC unconditionally
 *		(modulo ``jfile'' instructions)
 * FETCH(o,s)	returns whether instruction ``o'' is valid and has size ``s''
 *		(for ``s'' positive)
 * DELETE(o)	renders instruction ``o'' invalid
 * SETLABEL(o)	marks instruction ``o'' as requiring a label in the output
 */
#define NREQD(o)	(insts[o].flags & 3)
#define JUMP(o)		(insts[o].flags & (ISBRA | ISRTS | ISJMP | ISJSR))
#define FETCH(o,s)	(/* (o) >= 0 && */ (o) <= maxoffset && \
			  ((s) == -1 && insts[o].size \
			  || insts[o].size == (s)))
#define DELETE(o)	do { \
				free(insts[o].required); \
				insts[o].required = NULL; \
				insts[o].size = 0; \
				insts[o].flags = 0; \
			} while (0);
#define SETLABEL(o)	if ((o) >= initialpc && (o) <= initialpc + maxoffset) \
				insts[(o) - initialpc].flags |= ISLABEL

static short	longestinstsize;	/* max possible instruction length */
static long	gfsize = 0;		/* file size */
static addr_t	*notinsts = NULL;	/* array of values in ``nfile'' */
static size_t	nnotinsts = 0;
static addr_t	*breaks = NULL;		/* array of values in ``bfile'' */
static size_t	nbreaks = 0;
#ifndef NOBAD
static addr_t	*bad;	/* addresses that cause instructions to be deleted */
static addr_t	*good;	/* addresses that cause instructions to be accepted
			   in the final output */
#endif

/*
 * Free dynamically allocated memory
 * and longjmp back to process the next object file.
 */
static void
jumpfree(void)
{
#ifndef NOBAD
	addr_t	offset;

	for (offset = 0; offset <= maxoffset;
	  offset += (odd ? 1 : sizeof(word_t))) {
		if (dobad && insts[offset].size == 0)
			fprintf(stderr,
"%s: Instruction %lx: deleted because of %lx\n",
			  sfile, (long)(offset + initialpc), (long)bad[offset]);
		if (insts[offset].flags & ISGOOD
		  && bad[offset])
			fprintf(stderr,
"%s: Instruction %lx: assumed good because of %lx, deleted because of %lx\n",
			  sfile, (long)(offset + initialpc), (long)good[offset],
			  (long)bad[offset]);
	}
	free(bad);
	bad = NULL;
	free(good);
	good = NULL;
#endif
	while (gfsize--)
		free(insts[gfsize].required);
	free(insts);
	insts = NULL;
	gfsize = 0;
	longjmp(jmp, 1);
}

/*
 * Search ``p'' for a string of at least ``minlen'' consecutive
 * printable characters.  Return the index of the start of the string (or -1).
 */
static int
findstring(size_t stored, const unsigned char *p)
{
	int	i;
	int	inarow;

	for (inarow = i = 0; i < stored; i++, p++)
		if (isprint(*p)) {
			if (++inarow >= minlen)
				return i - inarow + 1;
		} else
			inarow = 0;

	return -1;
}

/*
 * ``p'' contains the nibbles of a floating-point constant in hexadecimal.
 * These nibbles are converted into longword values and passed to ``fpoint''
 * which formats the floating-point value in ``s'' (in printable form).
 */
static int
sfpoint(int type, const unsigned char *p, char *s)
{
	u32bit_t	longwords[3];
	size_t		nlongwords;
	size_t		i, j;

	switch (type) {
	case SINGLE:    nlongwords = 1; break;
	case DOUBLE:    nlongwords = 2; break;
	case EXTENDED: case PACKED:
			nlongwords = 3; break;
	}

	/*
	 * Convert string to longs.
	 */
	for (i = 0; i < nlongwords; i++) {
		longwords[i] = 0;
		for (j = 0; j < 2 * sizeof(word_t); j++)
			longwords[i] += (*p++)
			  << (CHAR_BIT * (2 * sizeof(word_t) - 1 - j));
	}

	return fpoint(longwords, type, s);
}

/*
 * Output constants.  Return how many floating-point constants were output.
 *
 * todo		number of input bytes to output.
 * p		nibbles of the constant(s) in hexadecimal.
 * floattype	floating-point type expected (or 0).
 * numforce	specifies how many floating-point constants should be output
 *		regardless of value (only used if ``floattype'' is nonzero.
 */
static int
dcflush(size_t todo, const unsigned char *p, int floattype, int numforce)
{
	char	format[BUFSIZ];
	int	i;
	addr_t	value;
	size_t	n;
	size_t	j;
	size_t	length;
	int	lflags;
	int	first = 1;
	int	dofp;
	int	rval = 0;/* return number of floating-point constants done */

	while (todo) {
		lflags = ops2f(1);
		if ((length = fsizeof(floattype)) == 0)
			floattype = 0;

		/*
		 * Determine if a floating-point constant should be output.
		 * After forced constants are output, we stay with the
		 * same type until we get NaN or Denormalized.
		 */
		dofp = 0;
		if (floattype && length <= todo) {
			if (sfpoint(floattype, p, format) != -1)
				dofp = 1;
		}
		if (--numforce >= 0 || dofp) {
			dofp = 1;
			lflags |= size2f(floattype);
		} else if (floattype)
			return rval;

		/*
		 * For integral constants, we do 2 words at a time.
		 * If the address is odd, we do the first byte by itself.
		 */
		if (!dofp) {
			length = 2 * sizeof(word_t);
			if (todo < length)
				length = todo;
			if (first && (ppc & 1) && length != todo)
				length = 1;
			if ((length % sizeof(word_t)) == 0) {
				lflags |= size2f(WORD);
				for (n = 0, i = 0; i < length / 2; i++) {
					for (value = 0, j = 0;
					  j < sizeof(word_t); j++)
						value += p[i * sizeof(word_t)
						  + j] << (CHAR_BIT
						  * (sizeof(word_t) - 1 - j));
						value = signextend(value, 16);
					if (i)
						n += sprintf(format + n, ",");
					n += sprintf(format + n, "#%d", value);
				}
			} else {
				lflags |= size2f(BYTE);
				for (n = 0, i = 0; i < length; i++) {
					if (i)
						n += sprintf(format + n, ",");
					n += sprintf(format + n, "#%u", p[i]);
				}
			}
		}
		leninstbuf = 0;
		for (i = 0; i < length; i++)
			leninstbuf += sprintf(instbuf + leninstbuf, "%02x",
			  p[i]);
		pc += length;
		instprint(lflags, "DC", format);
		todo -= length;
		p += length;
		if (floattype)
			rval++;
		first = 0;
	}

	return rval;
}

/*
 * Output a string.
 */
static size_t
ascflush(size_t stored, const unsigned char *p)
{
	char	format[BUFSIZ];
	size_t	length;
	size_t	i;
	size_t	n;
	size_t	left;
	size_t	nbytes;

	for (length = 0; length < stored; length++)
		if (!isprint(p[length]))
			break;

	format[0] = '\'';
	left = length;

	while (left) {
		n = 1;
		leninstbuf = 0;

		nbytes = (left > slenprint) ? slenprint : left;

		for (i = length - left; i < length - left + nbytes; i++) {
			leninstbuf += sprintf(instbuf + leninstbuf, "%02x",
			  p[i]);
			n += sprintf(format + n, "%c", p[i]);

			/*
			 * Double single quotes in strings.
			 */
			if (p[i] == '\'')
				format[n++] = '\'';
		}
		format[n++] = '\'';
		format[n++] = '\0';
		pc += nbytes;
		instprint(ops2f(1) | size2f(BYTE), "DC", format);
		left -= nbytes;
	}

	return length;
}

/*
 * Convert a floating-point-label type to a floating-point type.
 */
static int
fl2ftype(short flags)
{
	if (flags & ISLABEL) {
		if (flags & L_ISSINGLE)
			return SINGLE;
		if (flags & L_ISDOUBLE)
			return DOUBLE;
		if (flags & L_ISEXTENDED)
			return EXTENDED;
		if (flags & L_ISPACKED)
			return PACKED;
	}

	return 0;
}

/*
 * Output the ``stored'' input bytes contained in ``consts''.
 * If flags specifies a floating-point type, output floating-point
 * constants of that type as long as the input looks like them.
 * Output strings as appropriate.  Otherwise, output integral constants.
 */
static void
flush(size_t stored, const unsigned char *consts, short flags)
{
	size_t	length;
	size_t	flength;
	int	spos = -2;
	int	labelfptype = fl2ftype(flags);
	int	labelfpsize = fsizeof(labelfptype);
	int	first = 1;

	while (stored) {
		spos = findstring(stored, consts);
		if (first && labelfpsize && stored >= labelfpsize) {
			int	numfpsdone;

			if (spos == -1)
				length = stored / labelfpsize * labelfpsize;
			else if (spos > labelfpsize)
				length = spos / labelfpsize * labelfpsize;
			else
				length = labelfpsize;

			/*
			 * Force a floating-point constant.
			 */
			numfpsdone = dcflush(length, consts, labelfptype, 1);
			stored -= numfpsdone * labelfpsize;
			consts += numfpsdone * labelfpsize;
			first = 0;
			continue;
		}
		if (spos) {
			/*
			 * Output integral constant(s).
			 */
			length = (spos < 0) ? stored : spos;
			dcflush(length, consts, 0, 0);
			stored -= length;
			consts += length;
		}
		if (spos >= 0) {
			/*
			 * Output string.
			 */
			length = ascflush(stored, consts);
			stored -= length;
			consts += length;
		}
		labelfpsize = 0;
	}
}

/*
 * Read a word (and extension words as necessary) from the input file
 * and determine if it is a possible instruction.
 *
 * ``valid'' is set to signal this.
 */
static int
validinst(void)
{
	word_t	inst;

	valid = 0;

	if (nextword(&inst) == 0) {
		switch (inst >> 12) {
		case 0:
			bit_movep_immediate(inst);
			break;
		case 1:
			movebyte(inst);
			break;
		case 2:
			movelong(inst);
			break;
		case 3:
			moveword(inst);
			break;
		case 4:
			misc(inst);
			break;
		case 5:
			addq_subq_scc_dbcc_trapcc(inst);
			break;
		case 6:
			valid = bcc_bsr(inst);
			break;
		case 7:
			moveq(inst);
			break;
		case 8:
			or_div_sbcd(inst);
			break;
		case 9:
			sub_subx(inst);
			break;
		case 10:
			aline(inst);
			break;
		case 11:
			cmp_eor(inst);
			break;
		case 12:
			and_mul_abcd_exg(inst);
			break;
		case 13:
			add_addx(inst);
			break;
		case 14:
			shift_rotate_bitfield(inst);
			break;
		case 15:
			coprocessor(inst);
			if (!valid)
				fline(inst);
			break;
		}
	}

	return valid;
}

/*
 * Now that we know where the constants are, make another pass
 * to determine which of them are referenced using the PC-relative
 * addressing mode.
 */
static void
dcpass(void)
{
	addr_t	offset;

	pass = DCLABELSPASS;
#ifndef OLD
	if (fseek(infp, 0, SEEK_SET) == -1) {
		perror("fseek");
		jumpfree();
	}
#endif
	for (curoffset = offset = 0; offset <= maxoffset; )
		if (insts[offset].size) {
			if (curoffset != offset
			  && fseek(infp, curoffset = offset, SEEK_SET) == -1) {
				perror("fseek");
				jumpfree();
			}
			flags = 0;
			pc = ppc = offset + initialpc;
			leninstbuf = 0;
			validinst();
			offset += insts[offset].size;
		} else if (odd)
			offset++;
		else
			offset += sizeof(word_t);
}

/*
 * Make a pass over the input, outputting things as we currently see them.
 */
static void
printall(void)
{
	addr_t		offset;
	unsigned char	consts[BUFSIZ];
	size_t		stored = 0;

	if (fseek(infp, 0, SEEK_SET) == -1) {
		perror("fseek");
		jumpfree();
	}
	pc = ppc = initialpc;
	leninstbuf = 0;
	pass = DEBUGPASS;
	for (curoffset = offset = 0; offset < gfsize /* = maxoffset */;
	  offset += (odd ? 1 : sizeof(word_t))) {
		/*
		 * Determine if there might be a valid instruction
		 * which has the bytes at ``offset'' as operands.
		 */
		if (insts[offset].size == 0) {
			int	i = 0;
			size_t	size;

			for (size = odd ? 1 : sizeof(word_t);
			  size <= longestinstsize
			  && size <= offset;
			  size += (odd ? 1 : sizeof(word_t)))
				if (size < insts[offset - size].size) {
					i = 1;
					break;
				}
			if (i)
				continue;
		}

		if (curoffset != offset
		  && fseek(infp, curoffset = offset, SEEK_SET) == -1) {
			perror("fseek");
			jumpfree();
		}
		flags = 0;
		if (insts[offset].size) {
			if (stored) {
				flush(stored, consts, 0);
				stored = 0;
			}

			pc = ppc = initialpc + offset;
			leninstbuf = 0;
			validinst();
		} else {
			size_t	i;

			if (stored == 0)
				pc = ppc = initialpc + offset;
			for (i = 0; i < sizeof(word_t); i++)
				if (fread(consts + stored, 1, 1, infp) == 1)
					if (++stored >= sizeof consts) {
						flush(stored, consts, 0);
						stored = 0;
					}

		}
	}

	if (stored) {
		flush(stored, consts, 0);
		stored = 0;
	}

	fflush(NULL);
}

/*
 * Make the first pass over the input.
 * Gather references to other addresses.
 */
void
pass1(void)
{
	addr_t	offset;

	pass = FIRSTPASS;
	for (curoffset = offset = 0; offset <= maxoffset;
	  offset += (odd ? 1 : sizeof(word_t))) {
		if (curoffset != offset
		  && fseek(infp, curoffset = offset, SEEK_SET) == -1) {
			perror("fseek");
			jumpfree();
		}
		flags = 0;
		pc = ppc = offset + initialpc;
		leninstbuf = 0;
		if (validinst()) {
			/*
			 * It's a potential instruction.
			 */
			insts[offset].size = curoffset - offset;
			insts[offset].flags = flags;
			if (flags & 3) {
				addr_t	*reqd;

				if ((reqd = malloc((flags & 3) * sizeof(*reqd)))
				  == NULL) {
					perror("malloc");
					jumpfree();
				}
				switch (flags & 3) {
				case 3:	reqd[2] = required[2];
				case 2:	reqd[1] = required[1];
				case 1:	reqd[0] = required[0];
				}
				insts[offset].required = reqd;
			} else
				insts[offset].required = NULL;
#ifdef DEBUG
			if (debug & INITIAL) {
				int	i;

				fprintf(outfp,
				  "Writing offset = %lx, size = %d, flags = %d",
				  (long)offset, insts[offset].size,
				  insts[offset].flags);
				for (i = 0; i < (insts[offset].flags & 3); i++)
					fprintf(outfp, ", reqd = %lx",
					  (long)insts[offset].required[i]);
				fprintf(outfp, "\n");
			}
#endif
		}
		else {
#ifndef NOBAD
			bad[offset] = offset + initialpc;
#endif
#ifdef DEBUG
			if (debug & DELETIONS)
				fprintf(outfp, "0. Deleting offset %lx\n",
				  (long)offset);
#endif
		}
	}

#if !defined(NOBAD) || defined(DEBUG)
	for (offset = maxoffset + (odd ? 1 : sizeof(word_t));
	  offset <= maxoffset + longestinstsize;
	  offset += (odd ? 1 : sizeof(word_t))) {
#ifndef NOBAD
		if (offset <= maxoffset)
			bad[offset] = offset + initialpc;
#endif
#ifdef DEBUG
		if (debug & DELETIONS)
			fprintf(outfp, "0. Deleting offset %lx\n",
			  (long)offset);
#endif
	}
#endif
}

/*
 * Make a pass over the instruction database, checking for consistency.
 */
void
findbadrefs(void)
{
	long		offset;		/* must be signed */
	long		offset2;	/* must be signed */
	int		i;
	size_t		size;
	int		changes;
	static int	try = 1;

#ifdef DEBUG
	if (debug & TRY) {
		printall();
		fprintf(outfp, "\n\n\n");
	}
#endif

	/*
	 * Instructions that don't set PC
	 * must be followed by a valid instruction.
	 */
	do {
		changes = 0;
		for (offset = maxoffset - longestinstsize; offset >= 0;
		  offset -= (odd ? 1 : sizeof(word_t))) {
			/*
			 * Back up to a possible instruction.
			 * We do this to jump over a large data section.
			 */
			for (offset2 = offset; offset2 >= 0
			  && insts[offset2].size == 0;
			  offset2 -= (odd ? 1 : sizeof(word_t)))
				;
			if (offset2 < 0)
				break;
			if (offset2 + longestinstsize < offset)
				offset = offset2 + longestinstsize;

			if (!linkfallthrough && (insts[offset].flags & ISLINK)
			  || !FETCH(offset, -1)) {
				/*
				 * We've found an invalid instruction.
				 * See if any instructions advance PC here
				 * based on the size of the instruction
				 * and its operands.
				 */
				for (size = odd ? 1 : sizeof(word_t);
				  size <= longestinstsize
				  && size <= offset;
				  size += (odd ? 1 : sizeof(word_t)))
					if (FETCH(offset - size, size)
					  && !JUMP(offset - size)) {
#ifndef NOBAD
						if (!linkfallthrough
						  && (insts[offset].flags
						  & ISLINK))
							bad[offset - size]
							  = offset + initialpc;
						else
							bad[offset - size]
							  = bad[offset];
#endif
#ifdef DEBUG
						if (debug & DELETIONS)
fprintf(outfp,
  "1. Deleting offset %lx, size %d, flags = %d\n", (long)(offset - size),
  size, insts[offset - size].flags);
#endif
						DELETE(offset - size);
						changes++;
					}
			}
		}

		/*
		 * See if any instructions require
		 * an invalid instruction to be valid.
		 */
		for (offset2 = 0; offset2 <= maxoffset;
		  offset2 += (odd ? 1 : sizeof(word_t))) {
			if (insts[offset2].size == 0)
				continue;
			for (i = 0; i < NREQD(offset2); i++)
				if (insts[offset2].required[i] >= initialpc
				  && insts[offset2].required[i] <= initialpc
				  + maxoffset
				  && !FETCH(insts[offset2].required[i]
				  - initialpc, -1)) {
#ifndef NOBAD
					bad[offset2]
					  = bad[insts[offset2].required[i]
					  - initialpc];
#endif
#ifdef DEBUG
					if (debug & DELETIONS)
fprintf(outfp, "2. Deleting offset %lx, size %d because %lx is not valid\n",
(long)offset2, insts[offset2].size, (long)insts[offset2].required[i]);
#endif
					DELETE(offset2);
					changes++;
					break;
				}
		}

#ifdef DEBUG
		if (debug & TRY) {
			fprintf(outfp,
			  "TRY %d ###############################\n", try);
			printall();
			fprintf(outfp, "\n\n\n");
			try++;
		}
#endif
	} while (changes);

#ifdef DEBUG
	if (debug & LASTTRY) {
		fprintf(outfp, "TRY %d ###############################\n",
		  try - 1);
		printall();
		fprintf(outfp, "\n\n\n");
	}
#endif
}

struct queue {
	addr_t		address;
	struct queue	*next;
};

/*
 * The ``writeq'' and ``readq'' functions
 * maintain a queue of addresses for ``makegood''.
 */

static struct queue	head = { 0, NULL };

/*
 * Add to the queue.
 */
static void
writeq(addr_t address)
{
	struct queue	*tail;
	struct queue	*newq;

	if ((newq = malloc(sizeof(*newq))) == NULL) {
		perror("malloc");
		jumpfree();
	}
	newq->address = address;

#ifdef DEBUG
	if (debug & MAKEGOOD)
		fprintf(outfp, "Wrote offset = %lx\n", (long)address);
#endif

	newq->next = NULL;

	for (tail = &head; tail->next; tail = tail->next)
		;
	tail->next = newq;
}

/*
 * Read (and delete) from the queue.
 */
static struct queue *
readq(void)
{
	struct queue	*result;

	if (head.next) {
		result = head.next;
		head.next = head.next->next;

#ifdef DEBUG
		if (debug & MAKEGOOD)
			fprintf(outfp, "Read offset = %lx\n",
			  (long)result->address);
#endif
	} else
		result = NULL;

	return result;
}

/*
 * Mark ``offset'' as an instruction to be included in the final output.
 * Recursively mark as good all instructions that reference it.
 * Delete instructions that contradict those marked good.
 */
static void
makegood(addr_t offset)
{
	size_t		size;
	struct queue	*qptr = NULL;
	addr_t		origoffset = offset;

	if (insts[offset].flags & ISGOOD)
		return;

#ifdef DEBUG
	if (debug & MAKEGOOD)
		fprintf(outfp, "makegood(%lx)\n", (long)offset);
#endif

	do {
		if (qptr) {
			offset = qptr->address;
			free(qptr);
		}

#ifdef DEBUG
		if (debug & MAKEGOOD)
			fprintf(outfp, "Going with offset = %lx\n",
			  (long)offset);
#endif

		while (1) {

#ifdef DEBUG
			if (debug & MAKEGOOD)
				fprintf(outfp, "Offset = %lx\n", (long)offset);
#endif

			if (insts[offset].size == 0 ||
			  insts[offset].flags & ISGOOD)
				break;

			/*
			 * We have a ``good'' instruction.
			 * Instructions that overlap it should be deleted.
			 */
			for (size = odd ? 1 : sizeof(word_t);
			  size < longestinstsize
#ifndef OLD
			  && size + maxoffset >= offset
#endif
			  && size <= offset;
			  size += (odd ? 1 : sizeof(word_t)))
				if (insts[offset - size].size > size) {
#ifndef NOBAD
					bad[offset - size] = origoffset
					  + initialpc;
#endif
#ifdef DEBUG
					if (debug & DELETIONS)
fprintf(outfp, "3. Deleting offset %lx, size %d, flags = %d because of %lx\n",
  (long)(offset - size), insts[offset - size].size, insts[offset - size].flags,
  (long)offset);
#endif
#if 0
					if (insts[offset - size].flags & ISGOOD)
						fprintf(stderr,
"makegood(%lx): Deleting instruction previously assumed to be good: %lx\n",
						  (long)origoffset,
						  (long)(offset - size));
#endif
					DELETE(offset - size);
				}
			for (size = odd ? 1 : sizeof(word_t);
			  size < insts[offset].size
#ifndef OLD
			  && maxoffset >= offset + size;
#else
			  && size <= offset;
#endif
			  size += (odd ? 1 : sizeof(word_t)))
				if (insts[offset + size].size) {
#ifndef NOBAD
					bad[offset + size] = origoffset
					  + initialpc;
#endif
#ifdef DEBUG
					if (debug & DELETIONS)
fprintf(outfp, "4. Deleting offset %lx, size %d, flags = %d because of %lx\n",
  (long)(offset + size), insts[offset + size].size, insts[offset + size].flags,
  (long)offset);
#endif
#if 0
					if (insts[offset + size].flags & ISGOOD)
						fprintf(stderr,
"makegood(%lx): Deleting instruction previously assumed to be good: %lx\n",
						  (long)origoffset,
						  (long)(offset + size));
#endif
					DELETE(offset + size);
				}

			insts[offset].flags |= ISGOOD;
#ifndef NOBAD
			good[offset] = origoffset + initialpc;
#endif
			if ((insts[offset].flags & ISBRA)
			  || ((insts[offset].flags & ISJMP) &&
			  insts[offset].flags & 3)) {
				if (insts[offset].required[0] >= initialpc
				  && insts[offset].required[0] <= initialpc
				  + maxoffset)
					offset = insts[offset].required[0]
					  - initialpc;
				else
					break;
			} else if ((insts[offset].flags
			  & (ISBSR | ISJSR | ISBRA | ISBRcc | ISDBcc | ISJMP))
			  && (insts[offset].flags & 3)
			  && insts[offset].required[0] >= initialpc
			  && insts[offset].required[0] <= initialpc
			  + maxoffset) {
				writeq(insts[offset].required[0] - initialpc);
				offset += insts[offset].size;
			} else if (insts[offset].flags & (ISRTS | ISJMP))
				break;
			else
				offset += insts[offset].size;
		}
	} while (qptr = readq());
}

/*
 * Determine the number of remaining instances
 * of still-valid instructions overlapping each other.
 */
static long
overlaps(int list)
{
	long	num = 0;
	long	offset;
	size_t	size;

	for (offset = 0; offset <= maxoffset;
	  offset += (odd ? 1 : sizeof(word_t))) {
		if (insts[offset].size == 0)
			continue;

		for (size = odd ? 1 : sizeof(word_t);
		  size < longestinstsize && size <= offset;
		  size += (odd ? 1 : sizeof(word_t)))
			if (insts[offset - size].size > size) {
				num++;
				if (list)
					fprintf(stderr, "%s: Overlap at %lx\n",
					  sfile, offset);
			}
	}

	return num;
}

/*
 * Starting from ``offset'', return the next highest offset
 * containing an instruction marked good (or -1).
 */
static long
nextgood(addr_t offset)
{
	long	next;

	for (next = offset + (odd ? 1 : sizeof(word_t)); next <= maxoffset
	  && (insts[next].size == 0 || (insts[next].flags & ISGOOD) == 0);
	  next += (odd ? 1 : sizeof(word_t)))
		;

	return (next > maxoffset) ? -1 : next;
}

/*
 * Return whether program flow reaches address ``target''
 * starting from address ``prevonly''.
 */
static int
reach(addr_t prevonly, addr_t target)
{
	addr_t	offset;

	for (offset = prevonly; insts[offset].size && offset < target;
	  offset += insts[offset].size)
		;

	return offset == target;
}

/*
 * Fix the overlap situation.
 * Do this by minimizing the amount of data (constants) in the input.
 */
static void
fixoverlaps(void)
{
	addr_t	offset;
	addr_t	goodone = nextgood(0);

	for (offset = 0; offset <= maxoffset; ) {
		if (insts[offset].size == 0) {
			if (odd)
				offset++;
			else
				offset += sizeof(word_t);
			continue;
		}
		if (insts[offset].flags & ISGOOD) {
			offset += insts[offset].size;
			continue;
		}

		if (/*goodone >= 0 &&*/ goodone <= offset)
			goodone = nextgood(offset);

		if (reach(offset, goodone))
			makegood(offset);
		else if (odd)
			offset++;
		else
			offset += sizeof(word_t);
	}
}

static int
addrcmp(const void *p1, const void *p2)
{
	addr_t	a1 = *(addr_t *)p1;
	addr_t	a2 = *(addr_t *)p2;

	if (a1 < a2)
		return -1;
	else if (a1 > a2)
		return 1;
	else
		return 0;
}

/*
 * Return whether a ``math'' FPU instruction
 * has the word at ``offset'' as an extension word.
 */
static int
fpuoverlap(addr_t offset)
{
	size_t	size;

	if (!FPU(chip) || (insts[offset].flags & (ISBSR | ISBRA | ISBRcc)) == 0)
		return 0;

	for (size = 1; size < longestinstsize && size <= offset; size++)
		if (insts[offset - size].size > size
		  && insts[offset - size].flags & ISFPU)
			return 1;

	return 0;
}

/*
 * Disassemble!
 */
void
disassemble(void)
{
	addr_t		offset;
	long		fsize;
	int		changes;
	short		nlabels;
	int		i;
	unsigned char	consts[BUFSIZ];
	size_t		stored = 0;
	size_t		curnbreak;
	static int	breakssaved = 0;

	/*
	 * Determine the longest possible length of an instruction in words.
	 */
	switch (CPU(chip)) {
	case MC68000:
	case MC68010:
		longestinstsize = 10;
		break;
	case MC68020:
	case MC68030:
	default:
		if (FPU(chip))
			longestinstsize = 20;
		else
			longestinstsize = 20;
		break;
	}

	/*
	 * Determine the size of the input file.
	 */
	if (fseek(infp, 0, SEEK_END) == -1) {
		perror("fseek");
		jumpfree();
	}
	if ((gfsize = fsize = ftell(infp)) == -1) {
		perror("ftell");
		jumpfree();
	} else if (fsize < sizeof(word_t))
		jumpfree();
	if (fseek(infp, 0, SEEK_SET) == -1) {
		perror("fseek");
		jumpfree();
	}
	maxoffset = (fsize - (sizeof(word_t) - 1)) & ~(sizeof(word_t) - 1);

	/*
	 * Malloc and initialize instruction structures.
	 */
	if ((insts = malloc((fsize+1) * sizeof(*insts))) == NULL) {
		perror("malloc");
		jumpfree();
	}
#ifndef NOBAD
	if ((bad = malloc(fsize * sizeof(*bad))) == NULL
	  || (good = malloc(fsize * sizeof(*good))) == NULL) {
		perror("malloc");
		jumpfree();
	}
	memset(bad, '\0', fsize * sizeof(*bad));
	memset(good, '\0', fsize * sizeof(*good));
	if (!odd) {
		size_t	i;

		for (i = 1; i < fsize; i += 2)
			bad[i] = i + initialpc;
	}
#endif
	memset(insts, '\0', fsize * sizeof(*insts));
	while (fsize--)
		insts[fsize].required = NULL;

	/*
	 * Pass 1:
	 *
	 * Determine where the code is and where the data is.
	 */
	pass1();

	if (onepass == INCONSISTENT) {
		printall();
		jumpfree();
	}

	/*
	 * Process offsets specified by the user
	 * as places in data to start a new line of output.
	 */
	if (bfile && !breakssaved) {
		FILE	*bfp;

		if (bfp = fopen(bfile, "r")) {
			char		bbuf[80];
			unsigned long	ul;
			char		*cp;
			addr_t		*tmp;

			while (fgets(bbuf, sizeof bbuf, bfp)) {
				ul = strtoul(bbuf, &cp, 0);
				if (cp != bbuf && (odd || (ul & 1) == 0)) {
					if (ul >= initialpc && ul <= initialpc
					  + maxoffset) {
						offset = ul - initialpc;
						if (tmp = realloc(breaks,
						  (nbreaks + 1)
						  * sizeof(*breaks))) {
							breaks = tmp;
							breaks[nbreaks++]
							  = offset;
						}
					} else
						fprintf(outfp,
						  "File %s: bad pc: %s\n",
						  bfile, bbuf);
				} else
					fprintf(outfp,
					  "File %s: bad pc: %s\n", bfile, bbuf);
			}
			(void)fclose(bfp);
			qsort(breaks, nbreaks, sizeof(*breaks), addrcmp);
		} else
			perror(bfile);
		breakssaved = 1;
	}

	/*
	 * Process offsets specified by the user as being data.
	 */
	if (nfile) {
		FILE		*nfp;
		static int	notinstssaved = 0;

		if (nfp = fopen(nfile, "r")) {
			char		nbuf[80];
			unsigned long	ul;
			char		*cp;
			addr_t		*nottmp;
			size_t		size;

			while (fgets(nbuf, sizeof nbuf, nfp)) {
				ul = strtoul(nbuf, &cp, 0);
				if (cp != nbuf && (odd || (ul & 1) == 0)) {
					if (ul >= initialpc && ul <= initialpc
					  + maxoffset) {
						offset = ul - initialpc;
						if (nottmp = realloc(notinsts,
						  (nnotinsts + 1)
						  * sizeof(*notinsts))) {
							notinsts = nottmp;
							notinsts[nnotinsts++]
							  = offset;
						}

		for (size = 1; size < longestinstsize && size <= offset; size++)
			if (insts[offset - size].size > size) {
#ifndef NOBAD
				bad[offset - size] = (insts[offset].size)
				  ? offset + initialpc : bad[offset];
#endif
#ifdef DEBUG
				if (debug & DELETIONS)
fprintf(outfp,
  "5. Deleting offset %lx, size %d, flags = %d because %lx specified\n",
  (long)(offset - size), insts[offset - size].size, insts[offset - size].flags,
  (long)offset);
#endif
				DELETE(offset - size);
			}

						if (insts[offset].size) {
#ifndef NOBAD
							bad[offset] = offset
							  + initialpc;
#endif
#ifdef DEBUG
							if (debug & DELETIONS)
fprintf(outfp,
  "6. Deleting offset %lx, size %d, flags = %d because specified\n",
  (long)offset, insts[offset].size, insts[offset].flags);
#endif
							DELETE(offset);
						}
					} else
						fprintf(outfp,
						  "File %s: bad pc: %s\n",
						  nfile, nbuf);
				} else
					fprintf(outfp,
					  "File %s: bad pc: %s\n", nfile, nbuf);
			}
			(void)fclose(nfp);
			if (!notinstssaved) {
				qsort(notinsts, nnotinsts, sizeof(*notinsts),
				  addrcmp);
				notinstssaved = 1;
			}
		} else
			perror(nfile);
	}

	/*
	 * Process offsets specified by the user
	 * as not being the start of valid instructions.
	 */
	if (nsfile) {
		FILE		*nfp;
		static int	notinstssaved = 0;

		if (nfp = fopen(nsfile, "r")) {
			char		nbuf[80];
			unsigned long	ul;
			char		*cp;
			addr_t		*nottmp;
			size_t		size;

			while (fgets(nbuf, sizeof nbuf, nfp)) {
				ul = strtoul(nbuf, &cp, 0);
				if (cp != nbuf && (odd || (ul & 1) == 0)) {
					if (ul >= initialpc && ul <= initialpc
					  + maxoffset) {
						offset = ul - initialpc;
						if (nottmp = realloc(notinsts,
						  (nnotinsts + 1)
						  * sizeof(*notinsts))) {
							notinsts = nottmp;
							notinsts[nnotinsts++]
							  = offset;
						}

						if (insts[offset].size) {
#ifndef NOBAD
							bad[offset] = offset
							  + initialpc;
#endif
#ifdef DEBUG
							if (debug & DELETIONS)
fprintf(outfp,
  "6. Deleting offset %lx, size %d, flags = %d because specified\n",
  (long)offset, insts[offset].size, insts[offset].flags);
#endif
							DELETE(offset);
						}
					} else
						fprintf(outfp,
						  "File %s: bad pc: %s\n",
						  nsfile, nbuf);
				} else
					fprintf(outfp,
					  "File %s: bad pc: %s\n", nsfile,
					  nbuf);
			}
			(void)fclose(nfp);
			if (!notinstssaved) {
				qsort(notinsts, nnotinsts, sizeof(*notinsts),
				  addrcmp);
				notinstssaved = 1;
			}
		} else
			perror(nsfile);
	}

	/*
	 * Process offsets specified by the user as being instructions.
	 * Note that does *not* make an invalid instruction suddenly valid.
	 */
	if (ifile) {
		FILE	*ifp;

		if (ifp = fopen(ifile, "r")) {
			char		ibuf[80];
			unsigned long	ul;
			char		*cp;

			while (fgets(ibuf, sizeof ibuf, ifp)) {
				ul = strtoul(ibuf, &cp, 0);
				if (cp != ibuf && (odd || (ul & 1) == 0)) {
					if (ul >= initialpc && ul <= initialpc
					  + maxoffset)
						makegood(ul - initialpc);
					else
						fprintf(outfp,
						  "File %s: bad pc: %s\n",
						  ifile, ibuf);
				} else
					fprintf(outfp,
					  "File %s: bad pc: %s\n", ifile, ibuf);
			}
			(void)fclose(ifp);
		} else
			perror(ifile);
	}

	/*
	 * Instructions that don't set PC
	 * must be followed by a valid instruction.
	 *
	 * Instructions must reference valid instructions.
	 */
	findbadrefs();

	if (onepass == CONSISTENT) {
		printall();
		jumpfree();
	}

	/*
	 * Assume that all LINK instructions that are referenced
	 * by BSR or JSR instructions are valid.
	 */
	for (offset = 0; offset <= maxoffset;
	  offset += (odd ? 1 : sizeof(word_t)))
		if (insts[offset].size
		  && (insts[offset].flags & (ISBSR | ISJSR))
		  && (insts[offset].flags & 3)
		  && insts[offset].required[0] >= initialpc
		  && insts[offset].required[0] <= initialpc + maxoffset
		  && (insts[insts[offset].required[0] - initialpc].flags
		  & ISLINK)) {
#ifdef DEBUG
			if (debug & OVERLAPS)
				fprintf(outfp,
				  "Number of overlaps = %ld\n", overlaps(0));
#endif
			makegood(insts[offset].required[0] - initialpc);
		}

	findbadrefs();

#ifdef DEBUG
	if (debug & OVERLAPS)
		fprintf(outfp,
		  "Number of overlaps after LINKs with labels = %ld\n",
		  overlaps(0));
#endif

	/*
	 * Assume that all remaining LINK instructions are valid.
	 */
	for (offset = 0; offset <= maxoffset;
	  offset += (odd ? 1 : sizeof(word_t)))
		if (insts[offset].size
		  && (insts[offset].flags & ISGOOD) == 0
		  && (insts[offset].flags & ISLINK)) {
#ifdef DEBUG
			if (debug & OVERLAPS)
				fprintf(stderr, "Number of overlaps = %ld\n",
				  overlaps(0));
#endif
			makegood(offset);
		}

	findbadrefs();

#ifdef DEBUG
	if (debug & OVERLAPS)
		fprintf(stderr, "Number of overlaps after all LINKs = %ld\n",
		  overlaps(0));
#endif

	/*
	 * Assume that branch instructions that jump to valid instructions
	 * and that cannot be extension words of math FPU instructions
	 * are valid.
	 */
	do {
		changes = 0;
		for (offset = 0; offset <= maxoffset;
		  offset += (odd ? 1 : sizeof(word_t)))
			if (insts[offset].size
			  && (insts[offset].flags & ISGOOD) == 0
			  && (insts[offset].flags & (ISBSR | ISJSR | ISBRA
			  | ISBRcc | ISDBcc | ISJMP))
			  && (insts[offset].flags & 3)
			  && insts[offset].required[0] >= initialpc
			  && insts[offset].required[0] <= initialpc
			  + maxoffset
			  && (insts[insts[offset].required[0] - initialpc].flags
			  & ISGOOD)
			  && !fpuoverlap(offset)) {
#ifdef DEBUG
				if (debug & OVERLAPS)
					fprintf(stderr,
					  "Number of overlaps = %ld\n",
					  overlaps(0));
#endif
				makegood(offset);
				changes++;
			}
	} while (changes);

	findbadrefs();

#ifdef DEBUG
	if (debug & OVERLAPS)
		fprintf(stderr,
"Number of overlaps after all LINKs and jumps to good addresses = %ld\n",
	  overlaps(0));
#endif

	/*
	 * Assume that branch instructions that jump outside the current object
	 * file and that cannot be extension words of math FPU instructions
	 * are valid.
	 */
	do {
		changes = 0;
		for (offset = 0; offset <= maxoffset;
		  offset += (odd ? 1 : sizeof(word_t)))
			if (insts[offset].size
			  && (insts[offset].flags & ISGOOD) == 0
			  && (insts[offset].flags & (ISBSR | ISJSR | ISBRA
			  | ISBRcc | ISDBcc | ISJMP | ISRTS))
			  && !fpuoverlap(offset)) {
#ifdef DEBUG
				if (debug & OVERLAPS)
					fprintf(stderr,
					  "Number of overlaps = %ld\n",
					  overlaps(0));
#endif
				makegood(offset);
				changes++;
			}
	} while (changes);

	findbadrefs();

#ifdef DEBUG
	if (debug & OVERLAPS)
		fprintf(stderr,
		  "Number of overlaps after all LINKs and branches = %ld\n",
		  overlaps(0));
#endif

	findbadrefs();
	fixoverlaps();

#if 1
	{
		long	noverlaps = overlaps(1);

		if (noverlaps
#ifdef DEBUG
		  || debug & OVERLAPS
#endif
		  )
			fprintf(stderr, "%s: Number of overlaps = %ld\n", sfile,
			  noverlaps);
	}
#endif

	findbadrefs();

	/*
	 * Get the labels.
	 */
	dcpass();
	for (offset = 0; offset <= maxoffset; ) {
		if (insts[offset].size) {
			for (i = 0; i < NREQD(offset); i++)
				SETLABEL(insts[offset].required[i]);
			offset += insts[offset].size;
		} else if (odd)
			offset++;
		else
			offset += sizeof(word_t);
	}
	for (nlabels = 0, offset = 0; offset <= maxoffset; offset++)
		if (insts[offset].flags & ISLABEL) {
#ifdef DEBUG
			if (debug & LABELS)
				fprintf(outfp, "Label %d at %lx\n",
				  nlabels + 1, (long)offset);
#endif
			insts[offset].labelnum = ++nlabels;
		}

	/*
	 * Last pass: Print!
	 */
	if (fseek(infp, 0, SEEK_SET) == -1) {
		perror("fseek");
		jumpfree();
	}
	pc = ppc = initialpc;
	leninstbuf = 0;
	curnbreak = 0;
	pass = LASTPASS;
	for (stored = 0, curoffset = offset = 0; offset < gfsize; )  {
		if (curoffset != offset
		  && fseek(infp, curoffset = offset, SEEK_SET) == -1) {
			perror("fseek");
			jumpfree();
		}
		flags = 0;
		if (insts[offset].size) {
			if (stored) {
				flush(stored, consts,
				  insts[offset - stored].flags);
				stored = 0;
			}

			pc = ppc = initialpc + offset;
			leninstbuf = 0;
			validinst();
			offset = curoffset;
		} else {
			int	incrbreak;

			if (stored == 0)
				pc = ppc = initialpc + offset;

			/*
			 * Break the constant data up so that
			 * all labels are printed on a new line.
			 */
			while (curnbreak < nbreaks
			  && breaks[curnbreak] < offset)
				curnbreak++;
			incrbreak = curnbreak < nbreaks
			  && offset == breaks[curnbreak];
			if (insts[offset].flags & ISLABEL || incrbreak) {
				flush(stored, consts,
				  insts[offset - stored].flags);
				stored = 0;
				if (incrbreak)
					curnbreak++;
			}
			if (fread(consts + stored, 1, 1, infp) == 1)
				if (++stored >= sizeof consts) {
					flush(stored, consts,
					  insts[offset - stored].flags);
					stored = 0;
				}

			offset++;
		}
	}
	if (stored) {
		flush(stored, consts, insts[offset - stored].flags);
		stored = 0;
	}

	jumpfree();
	/* NOTREACHED */
}
