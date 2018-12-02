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
 * MC68851 PMMU support.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dis.h"
#include "addr.h"

static char	*pccs[] = {
	"BS", "BC", "LS", "LC", "SS", "SC", "AS", "AC",
	"WS", "WC", "IS", "IC", "GS", "GC", "CS", "CC"
};
#define NPCCS	(sizeof pccs / sizeof pccs[0])

char *
pcc(unsigned condition)
{
	return (condition < NPCCS) ? pccs[condition] : NULL;
}

static int
getfc(char *s, unsigned value)
{
	if (value == 0)
		strcpy(s, "SFC");
	else if (value == 1)
		strcpy(s, "DFC");
	else if (value & 0x0010) {
		if (!PMMU(chip) && (value & 8))
			return -1;
		sprintf(s, "FC%d", value & 0xf);
	} else if (value & 8)
		sprintf(s, "D%d", value & 0x7);
	else
		return -1;

	return 0;
}

static void
pload(word_t inst, unsigned long value)
{
	int	reg = inst & 7;
	int	mode = (inst >> 3) & 7;
	char	sload[7];

	if (!ISACEA(mode, reg))
		return;
	if (value & 0x01de0)
		return;
	if (getea(buf2, reg, mode, BYTE /* actually unsized */))
		return;
	sprintf(sload, "PLOAD%c", value & 0x0200 ? 'R' : 'W');
	if (getfc(buf1, (int)(value & 0x1f)) == -1)
		return;
	instprint(ops2f(2), sload, buf1, buf2);

	valid = 1;
}

static void
pvalid(word_t inst, unsigned long value)
{
	int	reg = inst & 7;
	int	mode = (inst >> 3) & 7;

	if (!ISACEA(mode, reg))
		return;

	if (value == 0x2800)
		strcpy(buf1, "VAL");
	else if ((value & 0xfff8) == 0x2c00)
		sprintf(buf1, "A%d", (int)(value & 7));
	else
		return;
	if (getea(buf2, reg, mode, BYTE /* actually unsized */))
		return;
	instprint(ops2f(2), "PVALID", buf1, buf2);

	valid = 1;
}

static void
pflush(word_t inst, unsigned long value)
{
	int	eareg = inst & 7;
	int	eamode = (inst >> 3) & 7;
	int	mode = (value >> 10) & 7;
	int	mask = (value >> 5) & 0xf;
	char	sflush[8];

	/*
	 * Should this be enforced when (mode & 2) == 0?
	 */
	if (!ISACEA(eamode, eareg))
		return;

	if (mode == 1) {
		if (value & 0x3ff)
			return;
		instprint(ops2f(0), "PFLUSHA");
	} else {
		if ((mask & 8) && CPU(chip) >= MC68030)
			return;
		immsprintf(buf2, mask);
		if (getfc(buf1, (int)(value & 0x1f)) == -1)
			return;
		strcpy(sflush, "PFLUSH");
		if (mode & 1) {
			if (CPU(chip) >= MC68030)
				return;
			strcat(sflush, "S");
		}
		if (mode & 2) {
			if (getea(buf3, eareg, eamode,
			  BYTE /* actually unsized */))
				return;
			instprint(ops2f(3) | sharp2f(2), sflush, buf1, buf2,
			  buf3);
		} else
			instprint(ops2f(2) | sharp2f(2), sflush, buf1, buf2);
	}

	valid = 1;
}

static void
pflushr(word_t inst, unsigned long value)
{
	int	reg = inst & 7;
	int	mode = (inst >> 3) & 7;

	if (value != 0xa000)
		return;
	if (!ISMEA(mode, reg))
		return;

	if (getea(buf1, reg, mode, BYTE /* actually unsized */))
		return;
	instprint(ops2f(1), "PFLUSHR", buf1);

	valid = 1;
}

static void
pmove(word_t inst, unsigned long value)
{
	int	reg = inst & 7;
	int	mode = (inst >> 3) & 7;
	int	pmmureg = (value >> 10) & 7;
	int	size;
	char	*cp1, *cp2;
	int	fd = value & 0x0100;

	/*
	 * Sizes:
	 *
	 * MC68851
	 * -------
	 * Double long:	CRP, SRP, DRP
	 * Long:	TC
	 * Word:	BAC, BAD, AC, PSR, PCSR
	 * Byte:	CAL, VAL, SCC
	 *
	 * MC68030
	 * -------
	 * Double long:	CRP, SRP
	 * Long:	TC, TT0, TT1
	 * Word:	MMUSR
	 */

	strcpy(buf3, "PMOVE");
	if (CPU(chip) >= MC68030) {
		if (!ISACEA(mode, reg))
			return;

		switch ((value >> 13) & 7) {
		case 0:
			if ((value & 0x18ff) != 0x0800)
				return;
			sprintf(buf1, "TT%d", pmmureg & 1);
			break;
		case 2:
			if (value & 0x10ff)
				return;
			switch (pmmureg) {
			case 0:
				strcpy(buf1, "TC");
				break;
			case 2:
				strcpy(buf1, "SRP");
				break;
			case 3:
				strcpy(buf1, "CRP");
				break;
			default:
				return;
			}
			break;
		case 3:
			if (value & 0x1dff)
				return;
			strcpy(buf1, "MMUSR");
			break;
		default:
			return;
		}
		if (fd) {
			if (value & 0x0200)
				return;
			strcat(buf3, "FD");
		}
	} else {
		/* MC68851 */
		if ((value & 0x2000) == 0) {
			if (pmmureg >= 1 && pmmureg <= 3
			  && (ISDATA(mode) || ISDIRECT(mode)))
				return;
	
			switch (pmmureg) {
			case 0:
				strcpy(buf1, "TC");
				size = LONGWORD;
				break;
			case 1:
				strcpy(buf1, "DRP");
				size = DOUBLELONGWORD;
				break;
			case 2:
				strcpy(buf1, "SRP");
				size = DOUBLELONGWORD;
				break;
			case 3:
				strcpy(buf1, "CRP");
				size = DOUBLELONGWORD;
				break;
			case 4:
				strcpy(buf1, "CAL");
				size = BYTE;
				break;
			case 5:
				strcpy(buf1, "VAL");
				size = BYTE;
				break;
			case 6:
				strcpy(buf1, "SCC");
				size = BYTE;
				break;
			case 7:
				strcpy(buf1, "AC");
				size = WORD;
				break;
			}
		} else {
			int	num = (value >> 2) & 7;

			if (!ISAEA(mode, reg))
				return;
	
			size = WORD;
			switch (pmmureg) {
			case 0:
				if (num)
					return;
				strcpy(buf1, "PSR");
				break;
			case 1:
				if (num || (value & 0x0200) == 0)
					return;
				strcpy(buf1, "PCSR");
				break;
			case 4:
				sprintf(buf1, "BAD%d", num);
				break;
			case 5:
				sprintf(buf1, "BAC%d", num);
				break;
			default:
				return;
			}
		}
	}
	if (getea(buf2, reg, mode, size))
		return;
	if (value & 0x0200) {
		cp1 = buf1;
		cp2 = buf2;
	} else {
		cp1 = buf2;
		cp2 = buf1;
	}

	instprint(ops2f(2), buf3, cp1, cp2);

	valid = 1;
}

static void
ptest(word_t inst, unsigned long value)
{
	int	reg = inst & 7;
	int	mode = (inst >> 3) & 7;
	int	level = (value >> 10) & 7;
	int	areg = (value >> 5) & 7;
	char	sareg[3];
	char	stest[7];

	if (!ISACEA(mode, reg))
		return;
	if (getea(buf2, reg, mode, BYTE /* actually unsized */))
		return;
	sprintf(stest, "PTEST%c", value & 0x0200 ? 'R' : 'W');
	if (getfc(buf1, (int)(value & 0x1f)) == -1)
		return;
	immsprintf(buf3, level);
	if (value & 0x0100) {
		sprintf(sareg, "A%d", areg);
		instprint(ops2f(4) | sharp2f(3), stest, buf1, buf2, buf3,
		  sareg);
	} else
		instprint(ops2f(3) | sharp2f(3), stest, buf1, buf2, buf3);

	valid = 1;
}

void
pgen(word_t inst)
{
	unsigned long	value;
	int		failure;

	value = getval(WORD, &failure) & 0xffff;
	if (failure)
		return;

	switch ((value >> 13) & 7) {
	case 0:
		if (CPU(chip) >= MC68030)
			pmove(inst, value);
		break;
	case 1:
		switch ((value >> 10) & 7) {
		case 0:
			pload(inst, value);
			break;
		case 2: /* FALLTHROUGH */
		case 3:
			if (PMMU(chip) == MC68851)
				pvalid(inst, value);
			break;
		default:
			pflush(inst, value);
			break;
		}
		break;
	case 2: /* FALLTHROUGH */
	case 3:
		pmove(inst, value);
		break;
	case 4:
		ptest(inst, value);
		break;
	case 5:
		if (PMMU(chip) == MC68851)
			pflushr(inst, value);
		break;
	default:
		return;
	}
}
