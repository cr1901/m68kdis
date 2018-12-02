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
 * Most of the functions that determine whether an instruction is valid
 * and then print it (as necessary) are here.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dis.h"
#include "addr.h"

void
bit_dynamic(word_t inst)
{
	int	destreg = inst & 7;
	int	destmode = (inst >> 3) & 7;
	int	type = (inst >> 6) & 3;
	int	srcreg = (inst >> 9) & 7;
	int	size = (destmode == 0) ? LONGWORD : BYTE;
	char	name[10];

	if (type == 0) {
		/* BTST */
		if (!ISDEA(destmode, destreg))
			return;
	} else {
		/* BCHG, BCLR, BSET */
		if (!ISADEA(destmode, destreg))
			return;
	}

	sprintf(name, "B%s", bitd[type]);
	if (getea(buf2, destreg, destmode, size))
		return;
	sprintf(buf1, "D%d", srcreg);
	instprint(ops2f(2) | size2f(size), name, buf1, buf2);

	valid = 1;
}

void
bit_static(word_t inst)
{
	int	reg = inst & 7;
	int	mode = (inst >> 3) & 7;
	int	type = (inst >> 6) & 3;
	int	size = (ISDATA(mode)) ? LONGWORD : BYTE;
	long	value;
	int	failure;
	char	name[10];

	if (type == 0) {
		/* BTST */
		if (!ISDEAlessIMM(mode, reg))
			return;
	} else {
		/* BCHG, BCLR, BSET */
		if (!ISADEA(mode, reg))
			return;
	}

	value = getval(/* BYTE */ WORD, &failure);
	if (failure)
		return;
	if (value & 0xff00)
		return;
	if (!ISDATA(mode))
		value %= 8;

	sprintf(name, "B%s", bitd[type]);
	if (getea(buf2, reg, mode, size))
		return;
	immsprintf(buf1, value);
	instprint(ops2f(2) | size2f(size) | sharp2f(1), name, buf1, buf2);

	valid = 1;
}

void
biti_reg(const char *name, int size, const char *reg)
{
	long	value;
	int	failure;

	value = getval(size, &failure);
	if (failure)
		return;
	immsprintf(buf1, value);
	instprint(ops2f(2) | size2f(size) | sharp2f(1), name, buf1, reg);

	if (size == BYTE && (value & 0xff00))
		return;

	valid = 1;
}

void
biti_size(const char *name, word_t inst)
{
	int	reg = inst & 7;
	int	mode = (inst >> 3) & 7;
	int	size = (inst >> 6) & 3;
	long	value;
	int	failure;

	if (name[0] == 'C') {
		/* CMPI */
		if (!ISDEAlessIMM(mode, reg))
			return;
	} else {
		/* ADDI, ANDI, EORI, ORI, SUBI */
		if (!ISADEA(mode, reg))
			return;
	}

	value = getval(size, &failure);
	if (failure)
		return;

	immsprintf(buf1, value);
	if (getea(buf2, reg, mode, size))
		return;
	instprint(ops2f(2) | size2f(size) | sharp2f(1), name, buf1, buf2);

	valid = 1;
}

void
cmp2_chk2(word_t inst)
{
	int	srcreg = inst & 7;
	int	srcmode = (inst >> 3) & 7;
	int	size = (inst >> 9) & 3;
	long	value;
	int	failure;
	int	destreg;

	if (!ISCEA(srcmode, srcreg))
		return;

	value = getval(WORD, &failure);
	if (failure)
		return;
	if (getea(buf1, srcreg, srcmode, size))
		return;
	destreg = (value >> 12) & 7;
	Areg2(buf2, (value & 0x8000) ? 'A' : 'D', destreg);
	instprint(ops2f(2) | size2f(size), (value & 0x0800) ? "CHK2" : "CMP2",
	  buf1, buf2);

	if (value & 0x07ff)
		return;

	valid = 1;
}

void
movep(word_t inst)
{
	int	addrreg = inst & 7;
	int	datareg = (inst >> 9) & 7;
	int	opmode = (inst >> 6) & 7;
	int	size = (opmode & 1) ? LONGWORD : WORD;
	long	value;
	int	failure;

	value = getval(WORD, &failure);
	if (failure)
		return;

	sprintf(buf1, "D%d", datareg);
	immsprintf(buf2, value);
	sprintf(buf2 + strlen(buf2), "(%2.2s)", Areg(addrreg));
	if (opmode & 2)
		instprint(ops2f(2) | size2f(size) | sharp2f(2), "MOVEP",
		  buf1, buf2);
	else
		instprint(ops2f(2) | size2f(size) | sharp2f(1), "MOVEP",
		  buf2, buf1);

	valid = 1;
}

void
cas(word_t inst)
{
	int	reg = inst & 7;
	int	mode = (inst >> 3) & 7;
	int	size;
	int	comparereg;
	int	updatereg;
	long	value;
	int	failure;

	if (!ISAMEA(mode, reg))
		return;

	switch ((inst >> 9) & 3) {
	case 1:	size = BYTE;		break;
	case 2:	size = WORD;		break;
	case 3:	size = LONGWORD;	break;
	}

	value = getval(WORD, &failure);
	comparereg = value & 7;
	updatereg = (value >> 6) & 7;
	if (failure)
		return;

	sprintf(buf1, "D%d", comparereg);
	sprintf(buf2, "D%d", updatereg);
	if (getea(buf3, reg, mode, size))
		return;
	instprint(ops2f(3) | size2f(size), "CAS", buf1, buf2, buf3);

	if (value & 0xfe38)
		return;

	valid = 1;
}

void
cas2(word_t inst)
{
	int	size = (inst & 0x0200) ? LONGWORD : WORD;
	long	value[2];
	int	failure;
	int	comparereg[2];
	int	updatereg[2];
	int	reg[2];
	int	anotd[2];
	int	i;

	for (i = 0; i < 2; i++) {
		value[i] = getval(WORD, &failure);
		if (failure)
			return;
		comparereg[i] = value[i] & 7;
		updatereg[i] = (value[i] >> 6) & 7;
		reg[i] = (value[i] >> 12) & 7;
		anotd[i] = (value[i] & 0x8000) ? 'A' : 'D';
	}

	sprintf(buf1, "D%d:D%d", comparereg[0], comparereg[1]);
	sprintf(buf2, "D%d:D%d", updatereg[0], updatereg[1]);
	Areg2(buf3, anotd[0], reg[0]);
	buf3[2] = ':';
	Areg2(&buf3[3], anotd[1], reg[1]);
	instprint(ops2f(3) | size2f(size), "CAS2", buf1, buf2, buf3);

	valid = 1;
}

void
moves(word_t inst)
{
	int	srcreg = inst & 7;
	int	srcmode = (inst >> 3) & 7;
	int	size = (inst >> 6) & 3;
	long	value;
	int	failure;
	int	reg;
	int	anotd;
	char	*cp1 = buf1, *cp2 = buf2;

	if (!ISAMEA(srcmode, srcreg))
		return;
	value = getval(WORD, &failure);
	if (failure)
		return;

	reg = (value >> 12) & 7;
	anotd = (value & 0x8000) ? 'A' : 'D';
	if (getea(buf1, srcreg, srcmode, size))
		return;
	Areg2(buf2, anotd, reg);
	if (value & 0x0800) {
		cp1 = buf2;
		cp2 = buf1;
	}

	instprint(ops2f(2) | size2f(size), "MOVES", cp1, cp2);

	if (value & 0x07ff)
		return;

	valid = 1;
}

void
move(word_t inst, int size)
{
	int	srcreg, destreg;
	int	srcmode, destmode;

	srcreg = inst & 7;
	srcmode = (inst >> 3) & 7;
	destmode = (inst >> 6) & 7;
	destreg = (inst >> 9) & 7;

	if (ISDIRECT(destmode)) {
		if (size == BYTE)
			return;
	} else if (size == BYTE && ISDIRECT(srcmode)
	  || !ISAEA(destmode, destreg))
		return;

	if (getea(buf1, srcreg, srcmode, size))
		return;
	if (ISDIRECT(destmode)) {
		sprintf(buf2, "%2.2s", Areg(destreg));
		instprint(ops2f(2) | size2f(size), "MOVEA", buf1, buf2);
	} else {
		if (getea(buf2, destreg, destmode, size))
			return;
		instprint(ops2f(2) | size2f(size), "MOVE", buf1, buf2);
	}

	valid = 1;
}

void
misc_size(const char *name, word_t inst)
{
	int	reg = inst & 7;
	int	mode = (inst >> 3) & 7;
	int	size = (inst >> 6) & 3;

	if (name[0] == 'T') {
		/* TST */
		if (size == BYTE && !ISDEAlessIMM(mode, reg))
			return;
	} else {
		/* CLR, NEG, NEGX, NOT */
		if (!ISADEA(mode, reg))
			return;
	}

	if (getea(buf1, reg, mode, size))
		return;
	instprint(ops2f(1) | size2f(size), name, buf1);

	valid = 1;
}

void
misc_ea(const char *name, word_t inst, int size)
{
	int	reg = inst & 7;
	int	mode = (inst >> 3) & 7;

	if (name[1] < 'C') {
		/* NBCD, TAS */
		if (!ISADEA(mode, reg))
			return;
	} else {
		/* JMP, JSR, PEA */
		if (!ISCEA(mode, reg))
			return;
	}
	if (getea(buf1, reg, mode, size))
		return;
	instprint(ops2f(1), name, buf1);

	valid = 1;
}

void
chk(word_t inst)
{
	int	srcreg = inst & 7;
	int	srcmode = (inst >> 3) & 7;
	int	destreg = (inst >> 9) & 7;

	if (!ISDEA(srcmode, srcreg))
		return;

	if (getea(buf1, srcreg, srcmode, WORD))
		return;
	sprintf(buf2, "D%d", destreg);
	instprint(ops2f(2), "CHK", buf1, buf2);

	valid = 1;
}

void
lea(word_t inst)
{
	int	srcreg = inst & 7;
	int	srcmode = (inst >> 3) & 7;
	int	destreg = (inst >> 9) & 7;
	int	retval;

	if (!ISCEA(srcmode, srcreg))
		return;
	retval = getea(buf1, srcreg, srcmode, LONGWORD);
	if (retval)
		return;
	sprintf(buf2, "%2.2s", Areg(destreg));
	instprint(ops2f(2), "LEA", buf1, buf2);

	valid = 1;
}

void
link(word_t inst, int size)
{
	int	reg = inst & 7;
	long	value;
	int	failure;

	value = getval(size, &failure);
	if (failure)
		return;
	sprintf(buf1, "%2.2s", Areg(reg));
	sprintf(buf2, "%ld", value);
	instprint(ops2f(2) | sharp2f(2), "LINK", buf1, buf2);

	valid = 1;
}

void
unlk(word_t inst)
{
	int	reg = inst & 7;

	sprintf(buf1, "%2.2s", Areg(reg));
	instprint(ops2f(1), "UNLK", buf1);

	valid = 1;
}

void
swap(word_t inst)
{
	int	reg = inst & 7;

	sprintf(buf1, "D%d", reg);
	instprint(ops2f(1), "SWAP", buf1);

	valid = 1;
}

void
bkpt(word_t inst)
{
	int	vector = inst & 0xf;

	sprintf(buf1, "%d", vector);
	instprint(ops2f(1) | sharp2f(1), "BKPT", buf1);

	valid = 1;
}

void
trap(word_t inst)
{
	int	vector = inst & 0xf;

	sprintf(buf1, "%d", vector);
	instprint(ops2f(1) | sharp2f(1), "TRAP", buf1);

	valid = 1;
}

void
stop_rtd(const char *name)
{
	int	value;
	int	failure;

	value = getval(WORD, &failure);
	if (failure)
		return;

	sprintf(buf1, "%ld", value);
	instprint(ops2f(1) | sharp2f(1), name, buf1);

	valid = 1;
}

void
movec(int tocr)
{
	long	value;
	int	failure;
	int	reg;
	int	anotd;
	int	controlreg;
	char	*cr;
	char	*cp1;
	char	*cp2;

	value = getval(WORD, &failure);
	if (failure)
		return;

	reg = (value >> 12) & 7;
	anotd = (value & 0x8000) ? 'A' : 'D';
	controlreg = value & 0x0fff;

	Areg2(buf1, anotd, reg);
	switch (controlreg) {
	case 0x000:	cr = "SFC";	break;	/* Source Function Code */
	case 0x001:	cr = "DFC";	break;	/* Destination Function Code */
	case 0x002:	cr = "CACR";	break;	/* Cache Control Register */
	case 0x800:	cr = "USP";	break;	/* User Stack Pointer */
	case 0x801:	cr = "VBR";	break;	/* Vector Base Register */
	case 0x802:	cr = "CAAR";	break;	/* Cache Address Register */
	case 0x803:	cr = "MSP";	break;	/* Master Stack Pointer */
	case 0x804:	cr = "ISP";	break;	/* Interrupt Stack Pointer */
	default:
		return;
	}

	if (tocr) {
		cp1 = buf1;
		cp2 = cr;
	} else {
		cp1 = cr;
		cp2 = buf1;
	}

	instprint(ops2f(2), "MOVEC", cp1, cp2);

	valid = 1;
}

void
ext(word_t inst)
{
	int	reg = inst & 3;
	int	opmode = (inst >> 6) & 3;
	int	size = (opmode == 2) ? WORD : LONGWORD;
	char	sext[5];

	sprintf(buf1, "D%d", reg);
	strcpy(sext, "EXT");
	if (inst & 0x0100)
		strcat(sext, "B");
	instprint(ops2f(1) | size2f(size), sext, buf1);

	valid = 1;
}

void
movereg(word_t inst, const char *regname, int to)
{
	int		reg = inst & 7;
	int		mode = (inst >> 3) & 7;
	const char	*cp1, *cp2;

	if (getea(buf1, reg, mode, WORD))
		return;
	if (to) {
		if (!ISDEA(mode, reg))
			return;
		cp1 = buf1;
		cp2 = regname;
	} else {
		if (!ISADEA(mode, reg))
			return;
		cp1 = regname;
		cp2 = buf1;
	}

	instprint(ops2f(2) | size2f(WORD), "MOVE", cp1, cp2);

	valid = 1;
}

void
moveusp(word_t inst, int to)
{
	int	reg = inst & 7;
	char	*cp1 = buf1, *cp2 = "USP";

	sprintf(buf1, "%2.2s", Areg(reg));
	if (!to) {
		cp1 = cp2;
		cp2 = buf1;
	}

	instprint(ops2f(2) | size2f(LONGWORD), "MOVE", cp1, cp2);

	valid = 1;
}

static void
reglist(char *s, unsigned long regmask, int mode)
{
	char	*t = s;

	if (mode == 4)
		revbits(&regmask, 16);
	s = regbyte(s, regmask & 0xff, "D", 0);
	s = regbyte(s, regmask >> 8, "A", s != t);
	if (s == t)
		strcpy(s, "0");
}

void
movem(word_t inst, int to)
{
	int		reg = inst & 7;
	int		mode = (inst >> 3) & 7;
	int		size = (inst & 0x40) ? LONGWORD : WORD;
	unsigned long	regmask;
	int		failure;
	char		*cp1, *cp2;

	regmask = getval(WORD, &failure) & 0xffff;
	if (failure)
		return;
	if (getea(buf1, reg, mode, size))
		return;
	reglist(buf2, regmask, mode);
	if (to) {
		if (!ISCEAplusPOST(mode, reg))
			return;
		cp1 = buf1;
		cp2 = buf2;
	} else {
		if (!ISACEAplusPRE(mode, reg))
			return;
		cp1 = buf2;
		cp2 = buf1;
	}

	instprint(ops2f(2) | size2f(size), "MOVEM", cp1, cp2);

	valid = 1;
}

void
dbcc(word_t inst)
{
	int	reg = inst & 7;
	int	condition = (inst >> 8) & 0xf;
	long	value;
	int	failure;
	char	sdbcc[5];
	addr_t	savedpc;
	short	f = ops2f(2);

	sprintf(sdbcc, "DB%s", condition == 1 ? "RA" : cc[condition]);
	sprintf(buf1, "D%d", reg);
	savedpc = pc;
	value = getval(WORD, &failure);
	if (failure)
		return;

	if (pass == FIRSTPASS && onepass != INCONSISTENT) {
		required[flags & 3] = value + savedpc;
		flags++;
	} else if (pass == LASTPASS && value + savedpc >= initialpc
	  && value + savedpc <= initialpc + maxoffset
	  && insts[value + savedpc - initialpc].labelnum)
		sprintf(buf2, "L%d",
		  insts[value + savedpc - initialpc].labelnum);
	else /* if (pass == DEBUGPASS
	  || value + savedpc > initialpc + maxoffset) */
		/* immsprintf(buf2, value); */
		sprintf(buf2, "%lx", (long)(value + savedpc));
	instprint(f, sdbcc, buf1, buf2);

	valid = 1;
}

void
trapcc(word_t inst)
{
	int	mode = inst & 7;
	int	condition = (inst >> 8) & 0xf;
	long	value = 0;
	int	failure;
	int	lflags;

	sprintf(buf1, "TRAP%s", cc[condition]);
	switch (mode) {
	case 2:
		value = getval(WORD, &failure);
		if (failure)
			return;
		lflags = ops2f(1) | size2f(WORD) | sharp2f(1);
		break;
	case 3:
		value = getval(LONGWORD, &failure);
		if (failure)
			return;
		lflags = ops2f(1) | size2f(LONGWORD) | sharp2f(1);
		break;
	case 4:
		lflags = ops2f(0);
		break;
	default:
		return;
	}

	sprintf(buf2, "%ld", value);

	instprint(lflags, buf1, buf2);

	valid = 1;
}

void
scc(word_t inst)
{
	int	reg = inst & 7;
	int	mode = (inst >> 3) & 7;
	int	condition = (inst >> 8) & 0xf;

	if (!ISADEA(mode, reg))
		return;
	sprintf(buf1, "S%s", cc[condition]);
	if (getea(buf2, reg, mode, BYTE))
		return;
	instprint(ops2f(1) | sharp2f(2), buf1, buf2);

	valid = 1;
}

void
pack_unpk(const char *name, word_t inst)
{
	int	reg1 = inst & 7;
	int	reg2 = (inst >> 9) & 7;
	long	value;
	int	failure;

	if (inst & 8) {
		sprintf(buf1, "-(%2.2s)", Areg(reg1));
		sprintf(buf2, "-(%2.2s)", Areg(reg2));
	} else {
		sprintf(buf1, "D%d", reg1);
		sprintf(buf2, "D%d", reg2);
	}

	value = getval(WORD, &failure);
	if (failure)
		return;
	immsprintf(buf3, value);
	instprint(ops2f(3) | sharp2f(3), name, buf1, buf2, buf3);

	valid = 1;
}

void
addq_subq(word_t inst)
{
	int	reg = inst & 7;
	int	mode = (inst >> 3) & 7;
	int	size = (inst >> 6) & 3;
	int	data;

	if (!ISAEA(mode, reg) || ISDIRECT(mode) && size == BYTE)
		return;

	if ((data = (inst >> 9) & 7) == 0)
		data = 8;
	sprintf(buf1, "%d", data);
	if (getea(buf2, reg, mode, size))
		return;
	instprint(ops2f(2) | size2f(size) | sharp2f(1),
	  (inst & 0x0100) ? "SUBQ" : "ADDQ", buf1, buf2);

	valid = 1;
}

void
op1(const char *name, word_t inst)
{
	int	datareg = (inst >> 9) & 7;
	int	reg = inst & 7;
	int	mode = (inst >> 3) & 7;
	int	size = (inst >> 6) & 3;
	char	*cp1, *cp2;

	if (getea(buf1, reg, mode, size))
		return;
	sprintf(buf2, "D%d", datareg);
	if (inst & 0x0100) {
		if (name[0] == 'E') {
			/* EOR */
			if (!ISADEA(mode, reg))
				return;
		} else {
			/* ADD, AND, OR, SUB */
			if (!ISAMEA(mode, reg))
				return;
		}
		cp1 = buf2;
		cp2 = buf1;
	} else {
		if (name[0] == 'O' || name[1] == 'N') {
			/* AND, OR */
			if (!ISDEA(mode, reg))
				return;
		} else {
			/* ADD, CMP, SUB */
			if (ISDIRECT(mode) && size == BYTE)
				return;
		}
		cp1 = buf1;
		cp2 = buf2;
	}
	instprint(ops2f(2) | size2f(size), name, cp1, cp2);

	valid = 1;
}

void
op2(const char *name, word_t inst)
{
	int	datareg = (inst >> 9) & 7;
	int	reg = inst & 7;
	int	mode = (inst >> 3) & 7;
	char	realname[10];

	if (!ISDEA(mode, reg))
		return;
	strcpy(realname, name);
	strcat(realname, ((inst >> 8) & 1) ? "S" : "U");
	if (getea(buf1, reg, mode, WORD))
		return;
	sprintf(buf2, "D%d", datareg);
	instprint(ops2f(2) | size2f(WORD), realname, buf1, buf2);

	valid = 1;
}

void
op2long(const char *name, word_t inst)
{
	int	reg = inst & 7;
	int	mode = (inst >> 3) & 7;
	char	realname[10];
	long	value;
	int	failure;
	int	rreg;
	int	qreg;
	int	quadword_dividend;

	if (!ISDEA(mode, reg))
		return;
	value = getval(WORD, &failure);
	if (failure)
		return;
	if (value & 0x83f8)
		return;

	rreg = value & 7;
	qreg = (value >> 12) & 7;
	quadword_dividend = value & 0x0400;

	strcpy(realname, name);
	strcat(realname, (value & 0x0800) ? "S" : "U");
	if (realname[0] == 'D' && rreg != qreg && !quadword_dividend)
		strcat(realname, "L");
	if (rreg == qreg && !quadword_dividend)
		sprintf(buf2, "D%d", qreg);
	else
		sprintf(buf2, "D%d:D%d", rreg, qreg);

	if (getea(buf1, reg, mode, LONGWORD))
		return;
	instprint(ops2f(2) | size2f(LONGWORD), realname, buf1, buf2);

	valid = 1;
}

void
opa(const char *name, word_t inst)
{
	int	addrreg = (inst >> 9) & 7;
	int	reg = inst & 7;
	int	mode = (inst >> 3) & 7;
	int	size = ((inst >> 8) & 1) ? LONGWORD : WORD;

	if (getea(buf1, reg, mode, size))
		return;
	sprintf(buf2, "%2.2s", Areg(addrreg));
	instprint(ops2f(2) | size2f(size), name, buf1, buf2);

	valid = 1;
}

void
opx(const char *name, word_t inst, int mode, int printsize)
{
	int	destreg = (inst >> 9) & 7;
	int	srcreg = inst & 7;
	int	size = (inst >> 6) & 3;
	int	lflags = ops2f(2);

	if (printsize)
		lflags |= size2f(size);

	if (inst & 8) {
		if (getea(buf1, srcreg, mode, size))
			return;
		if (getea(buf2, destreg, mode, size))
			return;
	} else {
		sprintf(buf1, "D%d", srcreg);
		sprintf(buf2, "D%d", destreg);
	}
	instprint(lflags, name, buf1, buf2);

	valid = 1;
}

void
exg(word_t inst, char c1, char c2)
{
	int	reg1 = (inst >> 9) & 7;
	int	reg2 = inst & 7;
	char	s[2];

	Areg2(buf1, c1, reg1);
	Areg2(buf2, c2, reg2);
	instprint(ops2f(2), "EXG", buf1, buf2);

	valid = 1;
}

void
bitfield(word_t inst)
{
	int	reg = inst & 7;
	int	mode = (inst >> 3) & 7;
	int	type = (inst >> 8) & 3;
	long	value;
	int	failure;
#ifndef OLD
	int	destreg;
	int	offset;
	int	width;
#else
	int	destreg = (value >> 12) & 7;
	int	offset = (value >> 6) & 0x1f;
	int	width = value & 0x1f;
#endif
	char	name[10];
	int	n;

	value = getval(WORD, &failure);
	if (failure)
		return;
#ifndef OLD
	if (value & 0x8000)
		return;
	if ((type & 1) == 0 && (value & 0xf000))
#else
	if ((type & 1) && (value & 0xf000))
#endif
		return;
#ifndef OLD
	destreg = (value >> 12) & 7;
	offset = (value >> 6) & 0x1f;
	width = value & 0x1f;
#endif
	sprintf(name, "BF%s", (type & 1) ? bitf[type] : bitd[type]);

	if (!ISDIRECT(mode))
		if (name[2] == 'C' || name[2] == 'I' || name[2] == 'S') {
			/* BFCHG, BFCLR, BFINS, BFSET */
			if (!ISACEA(mode, reg))
				return;
		} else {
			/* BFEXTS, BFEXTU, BFFFO, BFTST */
			if (!ISCEA(mode, reg))
				return;
		}

	if (getea(buf1, reg, mode, BYTE /* actually unsized */))
		return;
	strcpy(buf3, "{");
	n = 1;
	if (value & 0x0800) {
		if (offset & ~7)
			return;
		n += sprintf(buf3 + n, "D%d", offset & 7);
	} else
		n += sprintf(buf3 + n, "%d", offset);
	if (value & 0x0020) {
		if (width & ~7)
			return;
		n += sprintf(buf3 + n, "D%d", width & 7);
	} else
		n += sprintf(buf3 + n, "%d", width ? width : 32);
	strcat(buf3, "}");
	strcat(buf1, buf3);
	if (type & 1) {
		sprintf(buf2, "D%d", destreg);
		instprint(ops2f(2), name, buf1, buf2);
	} else
		instprint(ops2f(1), name, buf1);

	valid = 1;
}

void
getshiftname(char *name, int type, int direction)
{
	switch (type) {
	case 0:	strcpy(name, "AS");	break;
	case 1:	strcpy(name, "LS");	break;
	case 2:	strcpy(name, "ROX");	break;
	case 3:	strcpy(name, "RO");	break;
	}
	strcat(name, direction ? "L" : "R");
}

void
shift(word_t inst)
{
	int	reg;
	int	mode;
	int	type;
	int	direction = (inst >> 8) & 1;
	int	data;
	int	size;
	char	name[10];

	reg = inst & 7;
	if ((size = ((inst >> 6) & 3)) == 3) {
		size = WORD;
		mode = (inst >> 3) & 7;

		if (inst & 0x0800)
			return;
		if (!ISAMEA(mode, reg))
			return;

		if (getea(buf1, reg, mode, size))
			return;
		type = (inst >> 9) & 3;
		getshiftname(name, type, direction);
		instprint(ops2f(1), name, buf1);
	} else {
		sprintf(buf2, "D%d", reg);
		type = (inst >> 3) & 3;
		if (inst & 0x0020)
			sprintf(buf1, "D%d", (inst >> 9) & 7);
		else {
			if ((data = (inst >> 9) & 7) == 0)
				data = 8;
			sprintf(buf1, "#%d", data);
		}
		getshiftname(name, type, direction);
		instprint(ops2f(2) | size2f(size), name, buf1, buf2);
	}

	valid = 1;
}

void
cpsave(const char *prefix, word_t inst)
{
	int	reg = inst & 7;
	int	mode = (inst >> 3) & 7;

	if (!ISACEAplusPRE(mode, reg))
		return;
	if (getea(buf1, reg, mode, BYTE /* actually unsized */))
		return;
	sprintf(buf2, "%sSAVE", prefix);
	instprint(ops2f(1), buf2, buf1);

	valid = 1;
}

void
cprestore(const char *prefix, word_t inst)
{
	int	reg = inst & 7;
	int	mode = (inst >> 3) & 7;

	if (!ISCEAplusPOST(mode, reg))
		return;
	if (getea(buf1, reg, mode, BYTE /* actually unsized */))
		return;
	sprintf(buf2, "%sRESTORE", prefix);
	instprint(ops2f(1), buf2, buf1);

	valid = 1;
}

void
cpdbcc(struct cp *cpptr, word_t inst)
{
	int		reg = inst & 7;
	long		value;
	int		failure;
	char		sdbcc[6];
	addr_t		savedpc;
	short		f = ops2f(2);
	unsigned	condition;
	char		*condstr;

	savedpc = pc;
	condition = getval(WORD, &failure) & 0xffff;
	if (failure)
		return;
	if (condition & 0xffc0)
		return;
	if ((condstr = cpptr->cc(condition)) == NULL)
		return;
	sprintf(sdbcc, "%sDB%s", cpptr->prefix, condstr);
	sprintf(buf1, "D%d", reg);

	value = getval(WORD, &failure);
	if (failure)
		return;

	if (pass == FIRSTPASS && onepass != INCONSISTENT) {
		required[flags & 3] = value + savedpc;
		flags++;
	} else if (pass == LASTPASS && value + savedpc >= initialpc
	  && value + savedpc <= initialpc + maxoffset
	  && insts[value + savedpc - initialpc].labelnum)
		sprintf(buf2, "L%d",
		  insts[value + savedpc - initialpc].labelnum);
	else /* if (pass == DEBUGPASS
	  || value + savedpc > initialpc + maxoffset) */
		/* immsprintf(buf2, value); */
		sprintf(buf2, "%lx", (long)(value + savedpc));
	instprint(f, sdbcc, buf1, buf2);

	valid = 1;
}

void
cpbcc(struct cp *cpptr, word_t inst)
{
	unsigned	condition = inst & 0x003f;
	long		value;
	int		failure;
	char		sbcc[5];
	addr_t		savedpc;
	int		size = (inst & 0x0040) ? LONGWORD : WORD;
	char		*condstr;

	if ((condstr = cpptr->cc(condition)) == NULL)
		return;
	sprintf(sbcc, "%sB%s", cpptr->prefix, condstr);

	savedpc = pc;
	value = getval(size, &failure);
	if (failure)
		return;

	if (cpptr->prefix[0] == 'F' && cpptr->prefix[1] == '\0'
	  && inst == 0xf280 && value == 0) {
		instprint(ops2f(0), "FNOP");
		valid = 1;
		return;
	}

	if (onepass != INCONSISTENT
	  && (value < 0 && -value > savedpc - initialpc
          || value > 0 && value + savedpc > initialpc + maxoffset
          || !odd && value & 1))
                return;

	if (pass == FIRSTPASS && onepass != INCONSISTENT) {
		required[flags & 3] = value + savedpc;
		flags++;
	} else if (pass == LASTPASS && value + savedpc >= initialpc
	  && value + savedpc <= initialpc + maxoffset
	  && insts[value + savedpc - initialpc].labelnum)
		sprintf(buf1, "L%d",
		  insts[value + savedpc - initialpc].labelnum);
	else /* if (pass == DEBUGPASS
	  || value + savedpc > initialpc + maxoffset) */
		/* immsprintf(buf1, value); */
		sprintf(buf1, "%lx", (long)(value + savedpc));
	instprint(ops2f(1), sbcc, buf1);

	valid = 1;
}

void
cptrapcc(struct cp *cpptr, word_t inst)
{
	unsigned long	value;
	int		failure;
	int		lflags;
	int		mode = inst & 7;
	unsigned	condition;
	char		*condstr;

	condition = getval(WORD, &failure) & 0xffff;
	if (failure)
		return;
	if (condition & 0xffc0)
		return;
	if ((condstr = cpptr->cc(condition)) == NULL)
		return;
	sprintf(buf1, "%sTRAP%s", cpptr->prefix, condstr);

	switch (mode) {
	case 2:
		value = getval(WORD, &failure);
		if (failure)
			return;
		lflags = ops2f(1) | size2f(WORD) | sharp2f(1);
		break;
	case 3:
		value = getval(LONGWORD, &failure);
		if (failure)
			return;
		lflags = ops2f(1) | size2f(LONGWORD) | sharp2f(1);
		break;
	case 4:
		lflags = ops2f(0);
		break;
	default:
		return;
	}

	sprintf(buf2, "%ld", value);

	instprint(lflags, buf1, buf2);

	valid = 1;
}

void
cpscc(struct cp *cpptr, word_t inst)
{
	int		reg = inst & 7;
	int		mode = (inst >> 3) & 7;
	long		value;
	int		failure;
	unsigned	condition;
	char		*condstr;

	if (!ISADEA(mode, reg))
		return;
	condition = getval(WORD, &failure) & 0xffff;
	if (failure)
		return;
	if ((condstr = cpptr->cc(condition)) == NULL)
		return;

	if (getea(buf2, reg, mode, BYTE))
		return;

	sprintf(buf1, "%sS%s", cpptr->prefix, condstr);
	instprint(ops2f(1) | sharp2f(2), buf1, buf2);

	valid = 1;
}
