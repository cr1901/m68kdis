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
 * MC68881/MC68882 FPU support.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dis.h"
#include "addr.h"

static char	*fccs[] = {
	"F", "EQ", "OGT", "OGE", "OLT", "OLE", "OGL", "OR",
	"UN", "UEQ", "UGT", "UGE", "ULT", "ULE", "NE", "T",
	"SF", "SEQ", "GT", "GE", "LT", "LE", "GL", "GLE",
	"NGLE", "NGL", "NLE", "NLT", "NGE", "NGT", "SNE", "ST"
};
#define NFCCS	(sizeof fccs / sizeof fccs[0])

/*
 * Here come most of the math functions.
 * fmath accounts for those after FSUB.
 */
static char	*names[] = {
	"FMOVE", "FINT", "FSINH", "FINTRZ",
	"FSQRT", NULL, "FLOGNP1", NULL,
	"FETOXM1", "FTANH", "FATAN", NULL,
	"FASIN", "FATANH", "FSIN", "FTAN",
	"FETOX", "FTWOTOX", "FTENTOX", NULL,
	"FLOGN", "FLOG10", "FLOG2", NULL,
	"FABS", "FCOSH", "FNEG", NULL,
	"FACOS", "FCOS", "FGETEXP", "FGETMAN",
	"FDIV", "FMOD", "FADD", "FMUL",
	"FSGLDIV", "FREM", "FSCALE", "FSGLMUL",
	"FSUB"
};
#define NNAMES	(sizeof names / sizeof names[0])

char *
fcc(unsigned condition)
{
	return (condition < NFCCS) ? fccs[condition] : NULL;
}

int
flis2type(int is)
{
	switch (is) {
	case L_ISSINGLE:	return SINGLE;
	case L_ISDOUBLE:	return DOUBLE;
	case L_ISEXTENDED:	return EXTENDED;
	case L_ISPACKED:	return PACKED;
	default:		return -1;
	}
}

int
ftype2lis(int type)
{
	switch (type) {
	case SINGLE:	return L_ISSINGLE;
	case DOUBLE:	return L_ISDOUBLE;
	case EXTENDED:	return L_ISEXTENDED;
	case PACKED:	return L_ISPACKED;
	default:	return -1;
	}
}

size_t
fsizeof(int type)
{
	switch (type) {
	case SINGLE:	return 2 * sizeof(word_t);
	case DOUBLE:	return 4 * sizeof(word_t);
	case EXTENDED:	return 6 * sizeof(word_t);
	case PACKED:	return 6 * sizeof(word_t);
	default:	return 0;
	}
}

static int
srcreg2size(int srcreg)
{
	switch (srcreg) {
	case 0: return LONGWORD;
	case 1: return SINGLE;
	case 2: return EXTENDED;
	case 3: return PACKED;
	case 4: return WORD;
	case 5: return DOUBLE;
	case 6: return BYTE;
	default: return -1;
	}
}

static void
fmovefromfp(word_t inst, unsigned long value)
{
	int	size = (value >> 10) & 7;
	int	srcreg = (value >> 7) & 7;
	int	k = value & 0x7f;
	int	reg = inst & 7;
	int	mode = (inst >> 3) & 7;
	
	/*
	 * destreg and srcreg are switched so the
	 * call to srcreg2size is correct
	 * (size is playing the role of destreg)
	 */
	if ((size = srcreg2size(size)) == -1) {
		size = PACKED;
		if (k & 0x40)
			k |= ~0x7f;	/* sign-extend */
		sprintf(buf3, "{#%d}", k);
	} else if (size == PACKED) {
		if (k & 0xf)
			return;
		sprintf(buf3, "{D%d}", (int)(k >> 4));
	} else if (k)
		return;	/* not sure here: manual says k ``should'' be 0 */

	sprintf(buf1, "FP%d", srcreg);

#define ISSINGLE(s)	((s) == SINGLE)
	if (ISDATA(mode) && !ISINTEGRAL(size) && !ISSINGLE(size)
	  || !ISADEA(mode, reg))
		return;
#undef ISSINGLE
	if (getea(buf2, reg, mode, size))
		return;
	if (size == PACKED)
		strcat(buf2, buf3);

	instprint(ops2f(2) | size2f(size), "FMOVE", buf1, buf2);

	valid = 1;
}

static void
fmovelist(word_t inst, unsigned long value)
{
	int	to_ea = value & 0x2000;
	int	reg = inst & 7;
	int	mode = (inst >> 3) & 7;
	char	*cp1, *cp2;
	int	nregs;

	if (value & 0x3ff)
		return;
	if (to_ea && !ISADEA(mode, reg) && !ISDIRECT(mode))
		return;

	cp1 = buf1;
	*cp1 = '\0';
	nregs = 0;
	if (value & 0x1000) {
		cp1 += sprintf(cp1, "FPCR");
		nregs++;
	}
	if (value & 0x0800) {
		if (cp1 != buf1)
			*cp1++ = '/';
		cp1 += sprintf(cp1, "FPSR");
		nregs++;
	}
	if (value & 0x0400) {
		if (cp1 != buf1)
			*cp1++ = '/';
		cp1 += sprintf(cp1, "FPIAR");
		nregs++;
	}
	if (cp1 == buf1)
		strcpy(cp1, "0");
	
	if (ISDATA(mode) && nregs != 1
	  || ISDIRECT(mode) && nregs != 1 && (value & 0x0400) == 0)
		return;

	if (getea(buf2, reg, mode, LONGWORD))
		return;

	if (to_ea) {
		cp1 = buf1;
		cp2 = buf2;
	} else {
		cp1 = buf2;
		cp2 = buf1;
	}

	instprint(ops2f(2) | size2f(LONGWORD), nregs == 1 ? "FMOVE" : "FMOVEM",
	  cp1, cp2);

	valid = 1;
}

static void
fmovem(word_t inst, unsigned long value)
{
	int		to_ea = value & 0x2000;
	int		eareg = inst & 7;
	int		eamode = (inst >> 3) & 7;
	int		mode = (value >> 11) & 3;
	unsigned long	reglist = value & 0xff;
	char		*cp1, *cp2;

	if (value & 0x0700)
		return;

	if (to_ea) {
		if (!ISACEAplusPRE(eamode, eareg))
			return;
	} else if (!ISCEAplusPOST(eamode, eareg))
		return;

	/*
	 * Note that (mode & 2) implies postincrement or control
	 * addressing mode, otherwise predecrement addressing mode
	 *
	 * Not sure what this means though...
	 */
	if (mode & 1) {
		/*
		 * Dynamic list
		 */
		if (value & 0x8f)
			return;
		sprintf(buf1, "D%d", (int)((value >> 4) & 7));
	} else {
		/*
		 * Static list
		 */
		if (mode & 2)
			revbits(&reglist, 8);
		regbyte(buf1, reglist, "FP", 0);
	}

	if (getea(buf2, eareg, eamode, EXTENDED))
		return;

	if (to_ea) {
		cp1 = buf1;
		cp2 = buf2;
	} else {
		cp1 = buf2;
		cp2 = buf1;
	}

	instprint(ops2f(2) | size2f(EXTENDED), "FMOVEM", cp1, cp2);

	valid = 1;
}

static void
fmath(word_t inst, unsigned long value)
{
	int	reg = inst & 7;
	int	mode = (inst >> 3) & 7;
	int	srcreg = (value >> 10) & 7;
	int	destreg = (value >> 7) & 7;
	int	cosreg = value & 7;
	int	use_ea = value & 0x4000;
	int	size;
	size_t	name_index = value & 0x7f;
	int	nops;
	char	*name = NULL;

	size = EXTENDED;
	nops = 2;
	if (name_index == 0x3a) {
		name = "FTST";
		nops = 1;
		sprintf(buf2, "FP%d", destreg);
	} else if (name_index >= 0x30 && name_index <= 0x37) {
		name = "FSINCOS";
		sprintf(buf2, "FP%d:FP%d", cosreg, destreg /* sine reg */);
	} else {
		sprintf(buf2, "FP%d", destreg);
		if (name_index < NNAMES)
			name = names[name_index];
		else if (name_index == 0x38)
			name = "FCMP";
	}
	if (!name)
		return;

#define ISSINGLE(s)	((s) == SINGLE)
	if (use_ea) {
		if ((size = srcreg2size(srcreg)) == -1)
			return;
		if (ISDIRECT(mode) || ISDATA(mode) && !ISINTEGRAL(size)
		  && !ISSINGLE(size))
#undef ISSINGLE
			return;
		if (getea(nops == 1 ? buf2 : buf1, reg, mode, size))
			return;
	} else {
		if (mode || reg)
			return;
		if (srcreg == destreg && name_index < 0x20)
			nops = 1;
		else
			sprintf(buf1, "FP%d", srcreg);
	}

	if (nops == 1)
		instprint(ops2f(1) | size2f(size), name, buf2);
	else
		instprint(ops2f(2) | size2f(size), name, buf1, buf2);

	valid = 1;
}

static void
fmovecr(unsigned long value)
{
	int	reg = (value >> 7) & 7;
	int	offset = value & 0x7f;

	sprintf(buf2, "FP%d", reg);

	immsprintf(buf1, offset);
	switch (offset) {
	case 0x00:	strcat(buf1, "!PI");	break;
	case 0x0b:	strcat(buf1, "!Log10(2)");	break;
	case 0x0c:	strcat(buf1, "!e");	break;
	case 0x0d:	strcat(buf1, "!Log2(e)");	break;
	case 0x0e:	strcat(buf1, "!Log10(e)");	break;
	case 0x0f:	strcat(buf1, "!0.0");	break;
	case 0x30:	strcat(buf1, "!1n(2)");	break;
	case 0x31:	strcat(buf1, "!1n(10)");	break;
	case 0x32:	strcat(buf1, "!10^0");	break;
	case 0x33:	strcat(buf1, "!10^1");	break;
	case 0x34:	strcat(buf1, "!10^2");	break;
	case 0x35:	strcat(buf1, "!10^4");	break;
	case 0x36:	strcat(buf1, "!10^8");	break;
	case 0x37:	strcat(buf1, "!10^16");	break;
	case 0x38:	strcat(buf1, "!10^32");	break;
	case 0x39:	strcat(buf1, "!10^64");	break;
	case 0x3a:	strcat(buf1, "!10^128");	break;
	case 0x3b:	strcat(buf1, "!10^256");	break;
	case 0x3c:	strcat(buf1, "!10^512");	break;
	case 0x3d:	strcat(buf1, "!10^1024");	break;
	case 0x3e:	strcat(buf1, "!10^2048");	break;
	case 0x3f:	strcat(buf1, "!10^4096");	break;
	}

	instprint(ops2f(2), "FMOVECR", buf1, buf2);

	valid = 1;
}

void
fgen(word_t inst)
{
	unsigned long	value;
	int		failure;

	value = getval(WORD, &failure) & 0xffff;
	if (failure)
		return;

	if ((inst & 0x7f) == 0 && (value & 0xfc00) == 0x5c00)
		fmovecr(value);
	else
		switch ((value >> 13) & 7) {
		case 0: /* FALLTHROUGH */
		case 2:
			fmath(inst, value);
			break;
		case 3:
			fmovefromfp(inst, value);
			break;
		case 4: /* FALLTHROUGH */
		case 5:
			fmovelist(inst, value);
			break;
		case 6: /* FALLTHROUGH */
		case 7:
			fmovem(inst, value);
			break;
		default:
			return;
		}
}
