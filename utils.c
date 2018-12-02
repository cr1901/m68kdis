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
 * Various utility functions including formatting effective addresses,
 * reading input (correctly!), and printing are here.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include "dis.h"

/*
 * Format immediate constant ``value'' into ``s''.
 */
int
immsprintf(char *s, long value)
{
	long	absvalue;
	char	*sign;

	if (pass == FIRSTPASS)
		return 0;

	if (value < 0) {
		absvalue = -value;
		sign = "-$";
	} else {
		absvalue = value;
		sign = "$";
	}
	if (absvalue > 9)
		return sprintf(s, "%ld!%s%lx", value, sign, absvalue);
	else
		return sprintf(s, "%ld", value);
}

/*
 * Sign-extend ``value'' from a ``bits''-bit value to a long.
 */
long
signextend(long value, int bits)
{
	switch (bits) {
	case 8:
		value &= 0xff;
		if (value & 0x80)
			value |= ~0xffL;
		break;
	case 16:
		value &= 0xffff;
		if (value & 0x8000)
			value |= ~0xffffL;
		break;
	case 32:
		value &= 0xffffffff;
		if (value & 0x80000000)
			value |= ~0xffffffffL;
		break;
	}

	return value;
}

char	instbuf[512];
int	leninstbuf = 0;

/*
 * Read a word from the input file.  Put its numerical value in *wp.
 */
int
nextword(word_t *wp)
{
	unsigned char	c[sizeof(word_t)];
	size_t		i;

	if (fread(c, 1, sizeof(word_t), infp) == sizeof(word_t)) {
		for (*wp = 0, i = 0; i < sizeof(word_t); i++)
			*wp += c[i] << (CHAR_BIT * (sizeof(word_t) - 1 - i));
		pc += sizeof(word_t);
		curoffset += sizeof(word_t);
		for (i = 0; i < sizeof(word_t); i++) {
			sprintf(&instbuf[leninstbuf], "%0*x", sizeof(word_t),
			  c[i]);
			leninstbuf += sizeof(word_t);
		}
		return 0;
	} else
		return -1;
}

/*
 * Get a sign-extended value of type ``size'' from the input file.
 */
long
getval(int size, int *failure)
{
	word_t	extra[2];
	long	value;

	if (nextword(&extra[0]) == -1) {
		*failure = -1;
		return 0;
	}
	switch (size) {
	case BYTE:
#if 0
#ifndef OLD
		if (extra[0] & 0xff00) {
			*failure = -1;
			return 0;
		} else
#endif
#endif
		value = signextend(extra[0], 8);
		break;
	case WORD:
		value = signextend(extra[0], 16);
		break;
	case LONGWORD:
		if (nextword(&extra[1]) == -1) {
			*failure = -1;
			return 0;
		}
		value = signextend(((long)extra[0] << 16) | extra[1], 32);
		break;
	default:
		*failure = -1;
		return 0;
		break;
	}

	*failure = 0;
	return value;
}

/*
 * Translate scale bits to a scaling factor.
 */
static int
scale(int s)
{
	switch (s) {
	case 0:	return 1;	break;
	case 1:	return 2;	break;
	case 2:	return 4;	break;
	case 3:	return 8;	break;
	}
}

/*
 * Format address register ``reg'' taking ``sp'' into account.
 */
char *
Areg(int reg)
{
	static char	s[2] = "A0";

	if (sp && reg == 7)
		return "SP";
	else {
		s[1] = reg + '0';
		return s;
	}
}

/*
 * Format register ``reg''.
 */
void
Areg2(char *s, char c, int reg)
{
	if (c == 'A')
		sprintf(s, "%2.2s", Areg(reg));
	else
		sprintf(s, "D%d", reg);
}

/*
 * Extended mode for extension words.
 *
 * The formatted text goes in ``s''.
 * Return 0 for success, negative for failure.
 */
static int
extended(char *s, const char *reg, int size)
{
	word_t	extra;
	addr_t	bd;
	addr_t	od;
	addr_t	savedpc;
	int	comma;
	int	n = 0;
	int	failure;
	char	reg2[2];

	savedpc = pc;
	if (nextword(&extra) == -1)
		return -1;
	if (CPU(chip) < MC68020 || (extra & 0x0100) == 0) {
		/* Brief format */

		long	value;

		/*
		 * Format is as follows:
		 *
		 * Bits		Name
		 * 15		Index register type (D/A, 0 if D)
		 * 12-14	Index register number
		 * 11		Index size (W/L, 0 if sign-extended word)
		 * 9-10		Scale (00 = 1, 01 = 2, 10 = 4, 11 = 8)
		 * 8		Must be 0
		 * 0-7		Displacement
		 */

		if (CPU(chip) >= MC68020)
			n += sprintf(s, "(");
		value = (long)signextend(extra, 8);
		if (/* pcrelative && */ strcmp(reg, "PC") == 0)
			value += sizeof(word_t);
		n += immsprintf(s + n, value);
		n += sprintf(s + n, "%s", (CPU(chip) < MC68020) ? "(" : ",");
		Areg2(reg2, (extra & 0x8000) ? 'A' : 'D', (extra >> 12) & 7);
		n += sprintf(s + n, "%s,%2.2s.%c", reg, reg2,
		  (extra & 0x0800) ? 'L' : 'W');
		if (CPU(chip) >= MC68020 && ((extra >> 9) & 3))
			n += sprintf(s + n, "*%d", scale((extra >> 9) & 3));
		n += sprintf(s + n, ")");
	} else {
		/* Full format */

		/*
		 * Format is as follows:
		 *
		 * Bits		Name
		 * 15		Index register type (D/A, 0 if D)
		 * 12-14	Index register number
		 * 11		Index size (W/L, 0 if sign-extended word)
		 * 9-10		Scale (00 = 1, 01 = 2, 10 = 4, 11 = 8)
		 * 8		Must be 1
		 * 7		Base suppress (1 if base register suppressed)
		 * 6		Index suppress (1 if index register suppressed)
		 * 4-5		Base displacement size (00 = reserved,
				  01 = null, 10 = word, 11 = long)
		 * 3		Must be 0
		 * 0-2		Index/Indirect selection

		 * I/IS combinations:
		 *
		 * IS	I/IS	Operation
		 * 0	000	No memory indirection
		 * 0	001	Indirect preindexed with null outer displacement
		 * 0	010	Indirect preindexed with word od
		 * 0	011	Indirect preindexed with long od
		 * 0	101	Indirect postindexed with null od
		 * 0	110	Indirect postindexed with word od
		 * 0	111	Indirect postindexed with long od
		 * 1	000	No memory indirection
		 * 1	001	Memory indirect with null od
		 * 1	010	Memory indirect with word od
		 * 1	011	Memory indirect with long od
		 */

		/*
		 * Get base displacement
		 */
		switch ((extra >> 4) & 3) {
		case 0:
			return -1;
			break;
		case 1:
			bd = 0;
			break;
		case 2:
			bd = getval(WORD, &failure);
			if (failure)
				return failure;
			break;
		case 3:
			bd = getval(LONGWORD, &failure);
			if (failure)
				return failure;
			break;
		}

		/*
		 * Check if collapses to PC-relative
		 */
		if ((extra & 0x01cf) == 0x0140) {
			if (pass == DCLABELSPASS) {
				if (bd + savedpc >= initialpc
				  && bd + savedpc <= initialpc + maxoffset) {
					insts[bd + savedpc - initialpc].flags
					  |= ISLABEL;
					if (!insts[ppc - initialpc].size &&
					  insts[ppc - initialpc].flags & ISFPU)
						insts[bd + savedpc
						  - initialpc].flags
						  |= ftype2lis(size);
				}
			} else if (pass == FIRSTPASS && pcrelative) {
				required[flags & 3] = bd + savedpc;
				flags++;
			} else if (pass == LASTPASS
			  && bd + savedpc >= initialpc
			  && bd + savedpc <= initialpc + maxoffset
			  && insts[bd + savedpc - initialpc].labelnum)
				sprintf(s, "L%d",
				 insts[bd + savedpc - initialpc].labelnum);
			else /* if ((pass == FIRSTPASS || pass == LASTPASS)
			  && !pcrelative
			  || pass == DEBUGPASS
			  || pass == LASTPASS && pcrelative
			  && bd + savedpc > initialpc + maxoffset) */ {
				if (bd)
					sprintf(s, "(%ld,PC)!$%lx", bd,
					  bd + savedpc);
				else
					sprintf(s, "(PC)!$%lx", savedpc);
			}

			return 0;
		}

		switch (extra & 3) {
		case 0: /* FALLTHROUGH */
		case 1:
			od = 0;
			break;
		case 2:
			od = getval(WORD, &failure);
			if (failure)
				return failure;
			break;
		case 3:
			od = getval(LONGWORD, &failure);
			if (failure)
				return failure;
			break;
		}
		n += sprintf(s + n, "(");
		if (extra & 3)
			n += sprintf(s + n, "[");
		if (comma = bd)
			n += immsprintf(s + n, (long)bd);
		if ((extra & 0x0080) == 0) {
			/*
			 * Base suppress is 0.
			 */
			if (comma)
				n += sprintf(s + n, ",");
			n += sprintf(s + n, "%s", reg);
			comma = 1;
		} else if (strcmp(reg, "PC") == 0) {
			if (comma)
				n += sprintf(s + n, ",");
			n += sprintf(s + n, "ZPC");
			comma = 1;
		}
		if (extra & 4) {
			n += sprintf(s + n, "]");
			comma = 1;
		}
		if ((extra & 0x0040) == 0) {
			/*
			 * Index suppress is 0.
			 */
			if ((extra & 7) == 4)
				return -1;
			if (comma)
				n += sprintf(s + n, ",");
			Areg2(reg2, (extra & 0x8000) ? 'A' : 'D',
			  (extra >> 12) & 7);
			n += sprintf(s + n, "%2.2s.%c", reg2,
			  (extra & 0x0800) ? 'L' : 'W');
			if ((extra >> 9) & 3)
				n += sprintf(s + n, "*%d",
				  scale((extra >> 9) & 3));
		} else if (extra & 4)
			return -1;
		if ((extra & 3) && (extra & 4) == 0) {
			n += sprintf(s + n, "]");
			comma = 1;
		}
		if (od) {
			if (comma)
				n += sprintf(s + n, ",");
			n += immsprintf(s + n, (long)od);
		}
		if (n) {
			if (s[n - 1] != '(')
				n += sprintf(s + n, ")");
			else {
				s[n - 1] = '\0';
				if (--n == 0)
					sprintf(s, "0");
			}
		} else
			sprintf(s, "0");
	}

	return 0;
}

/*
 * The next few functions convert hexadecimal nibble values
 * to a floating-point value (and format it).
 */

#define BIT(p,n)        ((p)[(n) / 32] & (1UL << (32 - (n) % 32 - 1)))

/*
 * Is the mantissa zero?
 */
static int
zeromantissa(u32bit_t *p, size_t firstbit, size_t lastbit)
{
	size_t	bit;

	for (bit = firstbit; bit <= lastbit; bit++)
		if (BIT(p, bit))
			return 0;

	return 1;
}

/*
 * Convert the input data in *lwp from bits ``firstbit'' to ``lastbit''
 * into a mantissa.  Note that the *highest* bit is considered bit 0 here.
 */
static double
stod(u32bit_t *lwp, size_t firstbit, size_t lastbit)
{
	double		result = 1.0;
	double		value = 0.5;
	size_t		bit;

	for (bit = firstbit; bit <= lastbit && value; bit++) {
		if (BIT(lwp, bit))
			result += value;
		value /= 2.0;
	}

	return result;
}

/*
 * Convert the input data in *longwords into a floating-point value
 * of floating-point type ``type'' and place a human-readable version in ``s''.
 * Return -1 if NaN or Denormalized, else 0.
 */
int
fpoint(u32bit_t *longwords, int type, char *s)
{
	short	exponent;
	short	sign;
	short	zero;
	short	maxexp;
	short	minexp;
	size_t	firstbit;
	size_t	lastbit;
	int	rval = 0;	/* return -1 if NaN or Denormalized */

	/* These are for PACKED */
	short	mantissa[17];
	short	lastnonzero;
	short	firstnonzero;
	size_t	i, n;

	sign = (longwords[0] & 0x80000000) != 0;

	switch (type) {
	case SINGLE:
		/*
		 * Format:
		 *
		 * 1 bit	sign
		 * 8 bits	exponent (biased by 0x7f)
		 * 23 bits	mantissa (implicit leading bit
		 *			  left of implied binary point)
		 */
		exponent = ((longwords[0] >> (32 - (8+1))) & 0xff) - 0x7f;
		firstbit = 31 - 22;
		lastbit = 31;
		zero = zeromantissa(longwords, firstbit, lastbit);
		maxexp = 0x80;
		minexp = -0x7f;
		break;
	case DOUBLE:
		/*
		 * Format:
		 *
		 * 1 bit	sign
		 * 11 bits	exponent (biased by 0x3ff)
		 * 52 bits	mantissa (implicit leading bit
		 *			  left of implied binary point)
		 */
		exponent = ((longwords[0] >> (32 - (11+1))) & 0x7ff) - 0x3ff;
		firstbit = 63 - 51;
		lastbit = 63;
		zero = zeromantissa(longwords, firstbit, lastbit);
		maxexp = 0x400;
		minexp = -0x3ff;
		break;
	case EXTENDED:
		/*
		 * Format:
		 *
		 * 1 bit	sign
		 * 15 bits	exponent (biased by 0x3fff)
		 * 16 bits	unused
		 * 1 bit	explicit leading bit
		 *		left of binary point
		 * <binary point>
		 * 63 bits	mantissa (implicit leading bit
		 *			  left of implied binary point)
		 */
		exponent = ((longwords[0] >> (32 - (15+1))) & 0x7fff) - 0x3fff;
		firstbit = 95 - 62;
		lastbit = 95;
		zero = zeromantissa(longwords, firstbit, lastbit);
		maxexp = 0x4000;
		minexp = -0x3fff;
		break;
	case PACKED:
		/*
		 * Format:
		 *
		 * 1 bit	sign
		 * 1 bit	sign of exponent
		 * 2 bits	sign of exponent
		 * 12 bits	3-digit exponent
		 * 12 bits	unused
		 * 4 bits	explicit leading digit left of decimal point
		 * <decimal point>
		 * 64 bits	16-digit (17 with leading digit) mantissa
		 */
		zero = 1;
		firstnonzero = -1;
		for (i = 0; i < 17; i++) {
			mantissa[i] = longwords[(i + 7) / 8]
			  >> (((24 - i) % 8) * 4) & 0xf;
			if (mantissa[i]) {
				zero = 0;
				lastnonzero = i;
				if (firstnonzero == -1)
					firstnonzero = i;
			}
		}
		exponent = (longwords[0] >> 80) & 0xf
		  + ((longwords[0] >> 84) & 0xf) * 10
		  + ((longwords[0] >> 88) & 0xf) * 100;
		if (longwords[0] & 0x40000000)
			exponent = -exponent;
		exponent -= firstnonzero;
		break;
	}

	if (type != PACKED && exponent == maxexp
	  || type == PACKED && (longwords[0] & 0x7fff0000) == 0x7fff0000) {
		if (zero) {
			/* Infinity */
			if (sign)
				*s++ = '-';
			strcpy(s, "INF");
		} else {
			/* NaN */
			strcpy(s, "NaN");
		}
	} else if (type != PACKED && exponent == minexp
	  || type == PACKED && zero) {
		if (zero) {
			/* Zero */
			strcpy(s, "#0");
		} else {
			/* Denormalized */
			strcpy(s, "Denormalized");
			rval = -1;
		}
	} else if (type != PACKED) {
		/* Normalized */

		double	dmantissa;
		double	exp10;

		dmantissa = stod(longwords, firstbit, lastbit);
		if (type == EXTENDED && BIT(longwords, firstbit - 1) == 0) {
			/* Unnormalized */
			dmantissa -= 1.0;
			while (dmantissa < 1.0) {
				dmantissa *= 2.0;
				exponent--;
			}
		}

		*s++ = '#';
		if (sign)
			*s++ = '-';

		/*
		 * We need to determine if dmantissa * 2^exponent
		 * is within range or not.
		 */
		if (exponent < DBL_MAX_EXP && exponent > DBL_MIN_EXP
		  || exponent == DBL_MAX_EXP
		  && dmantissa <= DBL_MAX / pow(2.0, DBL_MAX_EXP)
		  || exponent == DBL_MIN_EXP
		  && dmantissa <= DBL_MIN / pow(2.0, DBL_MIN_EXP)
		  ) {
			if (exponent)
				dmantissa *= pow(2.0, exponent);
			sprintf(s, "%g", dmantissa);
		} else {
			double	exp10;
			double	log10value;

			log10value = log10(dmantissa)
			  + 0.30102999566398119521 * log10(exponent);
			exp10 = floor(log10value);
			dmantissa = pow(10.0, log10value - exp10);
			s += sprintf(s, "%g", dmantissa);
			if (exp10)
				sprintf(s, "e%d", dmantissa, (int)exp10);
		}
	} else /* type == PACKED */ {
		*s++ = '#';
		if (sign)
			*s++ = '-';
		s += sprintf(s, "%d", mantissa[firstnonzero]);
		if (lastnonzero != firstnonzero)
			*s++ = '.';
		for (i = firstnonzero + 1; i <= lastnonzero; i++)
			s += sprintf(s, "%d", mantissa[i]);

		if (exponent)
			sprintf(s, "e%d", exponent);
	}

	return rval;
}

/*
 * Read input bytes for floating-point constant
 * of floating-point type ``type''. Put formatted value in ``s''.
 */
static int
memfpoint(char *s, int type)
{
	u32bit_t	longwords[3];
	size_t		nlongwords;
	int	failure;
	size_t	i;

	switch (type) {
	case SINGLE:	nlongwords = 1;	break;
	case DOUBLE:	nlongwords = 2;	break;
	case EXTENDED: case PACKED:
			nlongwords = 3;	break;
	}

	for (i = 0; i < nlongwords; i++) {
		longwords[i] = getval(LONGWORD, &failure);
		if (failure)
			return failure;
	}

	return fpoint(longwords, type, s);
}

/*
 * Find effective address given mode/register combination and instruction size.
 * Result goes in ``s''.  Return 0 for success, else negative.
 */
int
getea(char *s, word_t reg, word_t mode, int size)
{
	long	longval;
	addr_t	savedpc;
	char	creg[3];
	int	failure;

	switch (mode) {
	case 0:
		/* Data register direct */
		sprintf(s, "D%d", reg);
		break;
	case 1:
		/* Address register direct */
		sprintf(s, "%2.2s", Areg(reg));
		break;
	case 2:
		/* Address register indirect */
		sprintf(s, "(%2.2s)", Areg(reg));
		break;
	case 3:
		/* Address register indirect with postincrement */
		sprintf(s, "(%2.2s)+", Areg(reg));
		break;
	case 4:
		/* Address register indirect with predecrement */
		sprintf(s, "-(%2.2s)", Areg(reg));
		break;
	case 5:
		/* Address register indirect with displacement */
		longval = getval(WORD, &failure);
		if (failure)
			return failure;
		if (CPU(chip) >= MC68020)
			sprintf(s, "(%ld,%2.2s)", longval, Areg(reg));
		else
			sprintf(s, "%ld(%2.2s)", longval, Areg(reg));
		break;
	case 6:
		/*
		 * Address register indirect
		 * with index and displacement
		 */
		sprintf(creg, "%2.2s", Areg(reg));
		return extended(s, creg, size);
		break;
	case 7:
		switch (reg) {
		case 0:
			/* Absolute short */
			longval = getval(WORD, &failure);
			if (failure)
				return failure;
			sprintf(s, "$%0*lx.W", 2 * sizeof(word_t), longval);
			break;
		case 1:
			/* Absolute long */
			longval = getval(LONGWORD, &failure);
			if (failure)
				return failure;
			sprintf(s, "$%0*lx.L", 4 * sizeof(word_t), longval);
			break;
		case 2:
			/* Program counter indirect with displacement */
			savedpc = pc;
			longval = getval(WORD, &failure);
			if (failure)
				return failure;

			if (pass == DCLABELSPASS) {
				if (longval + savedpc >= initialpc
				  && longval + savedpc <= initialpc
				  + maxoffset) {
					insts[longval + savedpc
					  - initialpc].flags |= ISLABEL;
					if (!insts[longval + savedpc
					  - initialpc].size &&
					  insts[ppc - initialpc].flags & ISFPU)
						insts[longval + savedpc
						  - initialpc].flags
						  |= ftype2lis(size);
				}
			} else if (pass == FIRSTPASS && pcrelative) {
				required[flags & 3] = longval + savedpc;
				flags++;
			} else if (pass == LASTPASS
			  && longval + savedpc >= initialpc
			  && longval + savedpc <= initialpc + maxoffset
			  && insts[longval + savedpc - initialpc].labelnum)
				sprintf(s, "L%d",
				 insts[longval + savedpc - initialpc].labelnum);
			else /* if ((pass == FIRSTPASS || pass == LASTPASS)
			  && !pcrelative
			  || pass == DEBUGPASS
			  || pass == LASTPASS && pcrelative
			  && longval + savedpc > initialpc + maxoffset) */ {
				if (longval == 0)
					sprintf(s, "(PC)!$%lx", savedpc);
				else if (CPU(chip) >= MC68020)
					sprintf(s, "(%ld,PC)!$%lx", longval,
					  longval + savedpc);
				else
					sprintf(s, "%ld(PC)!$%lx", longval,
					  longval + savedpc);
			}
			break;
		case 3:
			/*
			 * Program counter indirect
			 * with index and displacement
			 */
			return extended(s, "PC", size);
			break;
		case 4:
			/* Immediate */
			switch (size) {
			case BYTE:	/* FALLTHROUGH */
			case WORD:	/* FALLTHROUGH */
			case LONGWORD:	/* FALLTHROUGH */
			case DOUBLELONGWORD:
				s[0] = '#';
				longval = getval(size == DOUBLELONGWORD
				  ? LONGWORD : size, &failure);
				if (failure)
					return failure;
				s += immsprintf(s + 1, longval) + 1;
				if (size == DOUBLELONGWORD) {
					s[0] = '/';
					longval = getval(LONGWORD, &failure);
					if (failure)
						return failure;
					immsprintf(s + 1, longval);
				}
				break;
			case SINGLE:	/* FALLTHROUGH */
			case DOUBLE:	/* FALLTHROUGH */
			case EXTENDED:	/* FALLTHROUGH */
			case PACKED:
#if 0
				if (memfpoint(s, size) == -1)
					return -1;
#endif
				(void)memfpoint(s, size);
			}
			break;
		default:
			return -1;
		}
	}

	return 0;
}

/*
 * If printing is appropriate, print an output line.
 * The instruction name is given in ``name'' and ``lflags'' tells about
 * the instruction operands (the ellipsis arguments).
 */
void
instprint(int lflags, const char *name, ...)
{
	va_list ap;
	int	operands = f2ops(lflags);
	char	*cp;
	size_t	i;
	int	tabs, spaces;

	va_start(ap, name);

	if (pass == LASTPASS || pass == DEBUGPASS) {
		fprintf(outfp, "%08x", (int)ppc);
		if (lower)
			for (i = 0; i < leninstbuf; i++)
				instbuf[i] = tolower(instbuf[i]);
		fprintf(outfp, "   %.*s", (int)leninstbuf, instbuf);
#define TABSIZE	8
		if ((tabs = (36 - leninstbuf) / TABSIZE) <= 0)
			tabs = 1;
		while (tabs--)
			(void)putc('\t', outfp);
#undef TABSIZE

		if (pass == LASTPASS && insts[ppc - initialpc].labelnum)
			fprintf(outfp, "L%d", insts[ppc - initialpc].labelnum);
		(void)putc('\t', outfp);
		if (lower) {
			char	*newname;
			size_t	len = strlen(name);

			if (newname = malloc(len + 1)) {
				for (i = 0; i < len; i++)
					newname[i] = tolower(name[i]);
				newname[len] = '\0';
				fprintf(outfp, "%s", newname);
				free(newname);
			} else
				fprintf(outfp, "%s", name);
		} else
			fprintf(outfp, "%s", name);
		if (lower) {
			if (lflags & PBYTE)
				fprintf(outfp, ".b");
			else if (lflags & PWORD)
				fprintf(outfp, ".w");
			else if (lflags & PLONGWORD)
				fprintf(outfp, ".l");
			else if (lflags & PDOUBLELONGWORD)
				fprintf(outfp, ".dl");
			else if (lflags & PSINGLE)
				fprintf(outfp, ".s");
			else if (lflags & PDOUBLE)
				fprintf(outfp, ".d");
			else if (lflags & PEXTENDED)
				fprintf(outfp, ".x");
			else if (lflags & PPACKED)
				fprintf(outfp, ".p");
		} else {
			if (lflags & PBYTE)
				fprintf(outfp, ".B");
			else if (lflags & PWORD)
				fprintf(outfp, ".W");
			else if (lflags & PLONGWORD)
				fprintf(outfp, ".L");
			else if (lflags & PDOUBLELONGWORD)
				fprintf(outfp, ".DL");
			else if (lflags & PSINGLE)
				fprintf(outfp, ".S");
			else if (lflags & PDOUBLE)
				fprintf(outfp, ".D");
			else if (lflags & PEXTENDED)
				fprintf(outfp, ".X");
			else if (lflags & PPACKED)
				fprintf(outfp, ".P");
		}

		for (i = 1; i <= operands; i++) {
			putc(i == 1 ? '\t' : ',', outfp);
			if (lflags & sharp2f(i))
				putc('#', outfp);
			cp = va_arg(ap, char *);
			if (lower && (cp[0] != 'L' || !isdigit(cp[1]))) {
				char	*cp2 = cp - 1;

				while (*++cp2)
					*cp2 = tolower(*cp2);
			}
			fprintf(outfp, "%s", cp);
		}
		putc('\n', outfp);
	}

	leninstbuf = 0;
	ppc = pc;

	va_end(ap);
}

/*
 * Format a range of registers (``low'' to ``high'')
 * into ``s'', beginning with a ``slash'' if necessary.
 * ``ad'' specifies either address (A) or data (D) registers.
 */
static char *
regwrite(char *s, char *ad, int low, int high, int slash)
{
	if (slash)
		*s++ = '/';
	if (high - low > 1) {
		s += sprintf(s, "%s", ad);
		*s++ = low + '0';
		*s++ = '-';
		s += sprintf(s, "%s", ad);
		*s++ = high + '0';
	} else if (high - low == 1) {
		s += sprintf(s, "%s", ad);
		*s++ = low + '0';
		*s++ = '/';
		s += sprintf(s, "%s", ad);
		*s++ = high + '0';
	} else {
		s += sprintf(s, "%s", ad);
		*s++ = high + '0';
	}
	*s = '\0';

	return s;
}

/*
 * Format ``regmask'' into ``s''.  ``ad'' is a prefix used to indicate
 * whether the mask is for address, data, or floating-point registers.
 */
char *
regbyte(char *s, unsigned char regmask, char *ad, int doslash)
{
	int	i;
	int	last;

	for (last = -1, i = 0; regmask; i++, regmask >>= 1)
		if (regmask & 1) {
			if (last != -1)
				continue;
			else
				last = i;
		} else if (last != -1) {
			s = regwrite(s, ad, last, i - 1, doslash);
			doslash = 1;
			last = -1;
		}

	if (last != -1)
		s = regwrite(s, ad, last, i - 1, doslash);

	return s;
}

/*
 * Reverse the ``nbits'' bits in ``bits''.
 * Used to change register masks.
 */
void
revbits(unsigned long *bits, size_t nbits)
{
	int	i;
	int	b1, b2;

	for (i = 0; i < nbits / 2; i++) {
		b1 = *bits & (1 << i);
		b2 = *bits & (1 << (nbits - 1 - i));
		if (b1)
			*bits |= 1 << (nbits - 1 - i);
		else
			*bits &= ~(1 << (nbits - 1 - i));
		if (b2)
			*bits |= 1 << i;
		else
			*bits &= ~(1 << i);
	}
}
