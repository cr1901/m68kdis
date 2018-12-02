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
 * This file contains the functions called based on the high nibble
 * of the potentially valid instruction word.  Most of these call
 * functions in file inst2.c based on the remaining bits.
 */

#include <stdio.h>
#include <stdlib.h>
#include "dis.h"

void
bit_movep_immediate(word_t inst)
{
	if (inst == 0x003c)
		biti_reg("ORI", BYTE, "CCR");	/* 0000 0000 0011 1100 */
	else if (inst == 0x007c)
		biti_reg("ORI", WORD, "SR");	/* 0000 0000 0111 1100 */
	else if ((inst & 0xff00) == 0x0600)
		biti_size("ADDI", inst);	/* 0000 0110 ---- ---- */
	else if ((inst & 0xf9c0) == 0x00c0) {
		if (CPU(chip) >= MC68020)
			cmp2_chk2(inst);	/* 0000 0--0 11-- ---- */
	} else if ((inst & 0xff00) == 0)
		biti_size("ORI", inst);		/* 0000 0000 ---- ---- */
	else if (inst == 0x023c)
		biti_reg("ANDI", BYTE, "CCR");	/* 0000 0010 0011 1100 */
	else if (inst == 0x027c)
		biti_reg("ANDI", WORD, "SR");	/* 0000 0010 0111 1100 */
	else if ((inst & 0xff00) == 0x0200)
		biti_size("ANDI", inst);	/* 0000 0010 ---- ---- */
	else if ((inst & 0xff00) == 0x0400)
		biti_size("SUBI", inst);	/* 0000 0100 ---- ---- */
	else if ((inst & 0x0138) == 0x0108)
		movep(inst);			/* 0000 ---1 --00 1--- */
	else if (inst & 0x0100)
		bit_dynamic(inst);		/* 0000 ---1 ---- ---- */
	else if ((inst & 0xff00) == 0x0800)
		bit_static(inst);		/* 0000 1000 ---- ---- */
	else if ((inst & 0xfdff) == 0x0cfc) {
		if (CPU(chip) >= MC68020)
			cas2(inst);		/* 0000 11-0 1111 1100 */
	} else if ((inst & 0xf9c0) == 0x08fc) {
		if (CPU(chip) >= MC68020)
			cas(inst);		/* 0000 1--0 11-- ---- */
	} else if (inst == 0x0a3c)
		biti_reg("EORI", BYTE, "CCR");	/* 0000 1010 0011 1100 */
	else if (inst == 0x0a7c)
		biti_reg("EORI", WORD, "SR");	/* 0000 1010 0111 1100 */
	else if ((inst & 0xff00) == 0x0a00)
		biti_size("EORI", inst);	/* 0000 1010 ---- ---- */
	else if ((inst & 0xff00) == 0x0c00)
		biti_size("CMPI", inst);	/* 0000 1100 ---- ---- */
	else if ((inst & 0x0e00) == 0x0e00) {
		if (CPU(chip) >= MC68010)
			moves(inst);		/* 0000 1110 ---- ---- */
	}
		
}

void
movebyte(word_t inst)
{
	move(inst, BYTE);			/* 0001 ---- ---- ---- */
}

void
movelong(word_t inst)
{
	move(inst, LONGWORD);			/* 0010 ---- ---- ---- */
}

void
moveword(word_t inst)
{
	move(inst, WORD);			/* 0011 ---- ---- ---- */
}

void
misc(word_t inst)
{
	if (CPU(chip) >= MC68020 && (inst & 0x0140) == 0x0100
	  || CPU(chip) < MC68020 && (inst & 0x01c0) == 0x0180)
		chk(inst);			/* 0100 ---1 ?0-- ---- */
	else if ((inst & 0x0ff8) == 0x09c0) {
		if (CPU(chip) >= MC68020)
			ext(inst);	/* extb /* 0100 1001 1100 0--- */
	} else if ((inst & 0x01c0) == 0x01c0)
		lea(inst);			/* 0100 ---1 11-- ---- */
	else
		switch ((inst >> 8) & 0xf) {
		case 0:
			if ((inst & 0x00c0) == 0x00c0)
/* 0100 0000 11-- ---- */	movereg(inst, "SR", FROM);
			else
/* 0100 0000 ---- ---- */	misc_size("NEGX", inst);
			break;
		case 2:
			if ((inst & 0x00c0) == 0x00c0) {
				if (CPU(chip) >= MC68010)
/* 0100 0010 11-- ---- */		movereg(inst, "CCR", FROM);
			} else
/* 0100 0010 ---- ---- */	misc_size("CLR", inst);
			break;
		case 4:
			if ((inst & 0x00c0) == 0x00c0)
/* 0100 0100 11-- ---- */	movereg(inst, "CCR", TO);
			else
/* 0100 0100 ---- ---- */	misc_size("NEG", inst);
			break;
		case 6:
			if ((inst & 0x00c0) == 0x00c0)
/* 0100 0110 11-- ---- */	movereg(inst, "SR", TO);
			else
/* 0100 0110 ---- ---- */	misc_size("NOT", inst);
			break;
		case 8:
			if ((inst & 0x00f8) == 0x0008) {
/* 0100 1000 0000 1--- */	link(inst, LONGWORD);
				flags |= ISLINK;
			} else if ((inst & 0x00c0) == 0)
/* 0100 1000 00-- ---- */	misc_ea("NBCD", inst, BYTE);
			else if ((inst & 0x00f8) == 0x0040)
/* 0100 1000 0100 0--- */	swap(inst);
			else if ((inst & 0x00f8) == 0x0048) {
				if (CPU(chip) >= MC68010)
/* 0100 1000 0100 1--- */		bkpt(inst);
			} else if ((inst & 0x00c0) == 0x0040)
/* 0100 1000 01-- ---- */	misc_ea("PEA", inst, LONGWORD);
			else if ((inst & 0x00b8) == 0x0080)
/* 0100 1000 1-00 0--- */	ext(inst);
			else if ((inst & 0x0080) == 0x0080)
/* 0100 1000 1--- ---- */	movem(inst, FROM);
			break;
		case 10:
			if (inst == 0x4afc) {
/* 0100 1010 1111 1100 */	instprint(ops2f(0), "ILLEGAL");
				valid = 1;
			} else if ((inst & 0x00c0) == 0x00c0)
/* 0100 1010 11-- ---- */	misc_ea("TAS", inst, BYTE);
			else
/* 0100 1010 ---- ---- */	misc_size("TST", inst);
			break;
		case 12:
			if ((inst & 0x00c0) == 0) {
				if (CPU(chip) >= MC68020)
/* 0100 1100 00-- ---- */		op2long("MUL", inst);
			} else if ((inst & 0x00c0) == 0x0040) {
				if (CPU(chip) >= MC68020)
/* 0100 1100 01-- ---- */		op2long("DIV", inst);
			} else
/* 0100 1100 1--- ---- */	movem(inst, TO);
			break;
		case 14:
			switch ((inst >> 4) & 0xf) {
			case 4:
/* 0100 1110 0100 ---- */	trap(inst);
				break;
			case 5:
				if (inst & 8) {
/* 0100 1110 0101 1--- */		unlk(inst);
					flags |= ISUNLK;
				} else {
/* 0100 1110 0101 0--- */		link(inst, WORD);
					flags |= ISLINK;
				}
				break;
			case 6:
				if (inst & 8)
/* 0100 1110 0110 1--- */		moveusp(inst, FROM);
				else
/* 0100 1110 0110 0--- */		moveusp(inst, TO);
				break;
			case 7:
				switch (inst & 0xf) {
				case 0:
/* 0100 1110 0111 0000 */		instprint(ops2f(0), "RESET");
					valid = 1;
					break;
				case 1:
/* 0100 1110 0111 0001 */		instprint(ops2f(0), "NOP");
					valid = 1;
					break;
				case 2:
/* 0100 1110 0111 0010 */		stop_rtd("STOP");
					break;
				case 3:
/* 0100 1110 0111 0011 */		instprint(ops2f(0), "RTE");
					flags |= ISRTS;
					valid = 1;
					break;
				case 4:
					if (CPU(chip) >= MC68010) {
/* 0100 1110 0111 0100 */			stop_rtd("RTD");
						flags |= ISRTS;
						valid = 1;
					}
					break;
				case 5:
/* 0100 1110 0111 0101 */		instprint(ops2f(0), "RTS");
					flags |= ISRTS;
					valid = 1;
					break;
				case 6:
/* 0100 1110 0111 0110 */		instprint(ops2f(0), "TRAPV");
					valid = 1;
					break;
				case 7:
/* 0100 1110 0111 0111 */		instprint(ops2f(0), "RTR");
					flags |= ISRTS;
					valid = 1;
					break;
				default:
					if ((inst & 0xe) == 0xa
					  && CPU(chip) >= MC68010)
/* 0100 1110 0111 101- */			movec(inst & 1);
					break;
				}
				break;
			default:
				pcrelative = 1;
				if (inst & 0x0040) {
/* 0100 1110 11-- ---- */		misc_ea("JMP", inst, 0);
					flags |= ISJMP;
				} else {
/* 0100 1110 10-- ---- */		misc_ea("JSR", inst, 0);
					flags |= ISJSR;
				}
				pcrelative = 0;
				break;
			}
			break;
#ifdef __alpha
		/* Stupid DEC compiler */
		case 15:
			break;
#endif
		}
}

void
addq_subq_scc_dbcc_trapcc(word_t inst)
{
	if ((inst & 0x00c0) == 0x00c0) {
		if ((inst & 0x0038) == 0x0008) {
			dbcc(inst);		/* 0101 ---- 1100 1--- */
			flags |= ISDBcc;
		} else if ((inst & 0x0038) == 0x0038
		  && 2 <= (inst & 7) && (inst & 7) <= 4) {
			if (CPU(chip) >= MC68020)
				trapcc(inst);	/* 0101 ---- 1111 1--- */
		} else
			scc(inst);		/* 0101 ---- 11-- ---- */
	} else
		addq_subq(inst);		/* 0101 ---- ---- ---- */
}

int
bcc_bsr(word_t inst)
{
	long	value;
	int	condition = (inst >> 8) & 0xf;
	char	*cp;
	addr_t	savedpc;
	int	failure = 0;

	savedpc = pc;
	if ((value = signextend(inst & 0xff, 8)) == 0)
		value = getval(WORD, &failure);
	else if (CPU(chip) >= MC68020 && value == -1)
		value = getval(LONGWORD, &failure);
	if (failure)
		return 0;
	if (onepass != INCONSISTENT
	  && (value < 0 && -value > savedpc - initialpc
	  || value > 0 && value + savedpc > initialpc + maxoffset
	  || !odd && value & 1))
		return 0;

	switch (condition) {
	case 0:
		cp = "RA";		/* 0110 0000 ---- ---- */
		flags |= ISBRA;
		break;
	case 1:
		cp = "SR";		/* 0110 0001 ---- ---- */
		flags |= ISBSR;
		break;
	default:
		cp = cc[condition];	/* 0110 ---- ---- ---- */
		flags |= ISBRcc;
		break;
	}
	sprintf(buf1, "B%s", cp);

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
	instprint(ops2f(1), buf1, buf2);

	return 1;
}


void
moveq(word_t inst)	/* 0111 ---0 ---- ---- */
{
	int	reg = (inst >> 9) & 7;
	int	value;

	if (inst & 0x0100)
		return;
	value = signextend(inst & 0xff, 8);
	sprintf(buf1, "%d", value);
	sprintf(buf2, "D%d", reg);
	instprint(ops2f(2) | sharp2f(1), "MOVEQ", buf1, buf2);

	valid = 1;
}

void
or_div_sbcd(word_t inst)
{
	if ((inst & 0x01f0) == 0x0100)
		opx("SBCD", inst, 4, 0);	/* 1000 ---1 0000 ---- */
	else if ((inst & 0x01f0) == 0x0140)
		pack_unpk("PACK", inst);	/* 1000 ---1 0100 ---- */
	else if ((inst & 0x01f0) == 0x0180)
		pack_unpk("UNPK", inst);	/* 1000 ---1 1000 ---- */
	else if ((inst & 0x00c0) == 0x00c0)
		op2("DIV", inst);		/* 1000 ---- 11-- ---- */
	else
		op1("OR", inst);		/* 1000 ---- ---- ---- */
}

void
sub_subx(word_t inst)
{
	if ((inst & 0x00c0) == 0x00c0)
		opa("SUBA", inst);		/* 1001 ---- 11-- ---- */
	else if ((inst & 0x0130) == 0x0100)
		opx("SUBX", inst, 4, 1);	/* 1001 ---1 --00 ---- */
	else
		op1("SUB", inst);		/* 1001 ---- ---- ---- */
}

void
unimplemented(word_t inst)
{
	instprint(ops2f(0), "UNIMPLEMENTED");	/* 1010 ---- ---- ---- */
}

void
cmp_eor(word_t inst)
{
	if ((inst & 0x00c0) == 0x00c0)
		opa("CMPA", inst);		/* 1011 ---- 11-- ---- */
	else if ((inst & 0x0100) == 0)
		op1("CMP", inst);		/* 1011 ---0 ---- ---- */
	else if ((inst & 0x0038) == 8)
		opx("CMPM", inst, 3, 1);	/* 1011 ---1 --00 1--- */
	else
		op1("EOR", inst);		/* 1011 ---1 ---- ---- */
}

void
and_mul_abcd_exg(word_t inst)
{
	if ((inst & 0x00c0) == 0x00c0)
		op2("MUL", inst);		/* 1100 ---- 11-- ---- */
	else if ((inst & 0x01f0) == 0x0100)
		opx("ABCD", inst, 4, 0);	/* 1100 ---1 0000 ---- */
	else
		switch (inst & 0x01f8) {
		case 0x0140:
			exg(inst, 'D', 'D');	/* 1100 ---1 0100 0--- */
			break;
		case 0x0148:
			exg(inst, 'A', 'A');	/* 1100 ---1 0100 1--- */
			break;
		case 0x0188:
			exg(inst, 'D', 'A');	/* 1100 ---1 1000 1--- */
			break;
		default:
			op1("AND", inst);	/* 1100 ---- ---- ---- */
		}
}

void
add_addx(word_t inst)
{
	if ((inst & 0x00c0) == 0x00c0)
		opa("ADDA", inst);		/* 1101 ---- 11-- ---- */
	else if ((inst & 0x0130) == 0x0100)
		opx("ADDX", inst, 4, 1);	/* 1101 ---1 --00 ---- */
	else
		op1("ADD", inst);		/* 1101 ---- ---- ---- */
}

void
shift_rotate_bitfield(word_t inst)
{
	if ((inst & 0x08c0) == 0x08c0) {
		if (CPU(chip) >= MC68020)
			bitfield(inst);		/* 1110 1--- 11-- ---- */
	} else
		shift(inst);			/* 1110 ---- ---- ---- */
}

/*
 * Coprocessor support.  Currently only the PMMU and FPU available.
 */

struct cp	coproc[] = {
	{ "P",	pgen, pcc },
	{ "F",	fgen, fcc },
};
#define NCP	(sizeof coproc / sizeof coproc[0])

void
coprocessor(word_t inst)
{
	int		num = (inst >> 9) & 7;
	struct cp	*cpptr;
	const char	*prefix;

	if (num >= NCP || !coproc[num].prefix)
		return;

	cpptr = &coproc[num];
	prefix = cpptr->prefix;

	if (num == 0 && PMMU(chip) != MC68851
	  && (CPU(chip) < MC68030 || ((inst >> 6) & 7)))
		return;
	if (num == 1 && !FPU(chip))
		return;
	
	switch ((inst >> 6) & 7) {
	case 0:
		(*cpptr->gen)(inst);	/* 1111 num0 00-- ---- */
		if (valid && num == 1)
			flags |= ISFPU;
		break;
	case 1:
		if ((inst & 0x0038) == 0x0038)
			cptrapcc(cpptr, inst);	/* 1111 num0 0111 1--- */
		else if ((inst & 0x0038) == 0x0008)
			cpdbcc(cpptr, inst);	/* 1111 num0 0100 1--- */
		else
			cpscc(cpptr, inst);	/* 1111 num0 01-- ---- */
		break;
	case 2:
	case 3:
		cpbcc(cpptr, inst);		/* 1111 num0 1--- ---- */
		break;
	case 4:
		cpsave(prefix, inst);		/* 1111 num1 00-- ---- */
		break;
	case 5:
		cprestore(prefix, inst);	/* 1111 num1 01-- ---- */
		break;
	}
}
