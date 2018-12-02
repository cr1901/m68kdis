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
 * This file contains function prototypes, global variable declarations,
 * and #defined masks and macros for manipulating the flags member
 * of a struct inst.
 */

#ifndef DIS_H
#define DIS_H

#include <stdio.h>
#include <limits.h>
#include <setjmp.h>

/* Eight bits here */
#define BYTE		0x00
#define WORD		0x01
#define LONGWORD	0x02
#define DOUBLELONGWORD	0x04
#define SINGLE		0x08
#define DOUBLE		0x10
#define EXTENDED	0x20
#define PACKED		0x40
#define PWORD		WORD
#define PLONGWORD	LONGWORD
#define PDOUBLELONGWORD	DOUBLELONGWORD
#define PSINGLE		SINGLE
#define PDOUBLE		DOUBLE
#define PEXTENDED	EXTENDED
#define PPACKED		PACKED
#define PBYTE		0x80
#define size2f(s)	((s) == BYTE ? PBYTE : (s))
#define ISINTEGRAL(s)	((s) <= DOUBLELONGWORD)

/* Four bits here */
#define MAXOPSMASK	15	/* 4 bits => 1111 base 2 */
#define f2sharps(f)	(((f) >> 8) & MAXOPSMASK)
#define sharp2f(arg)	(1 << (8 + (arg) - 1))

#define f2ops(f)	((f) >> 12)
#define ops2f(n)	((n) << 12)

#define FROM		0
#define TO		1

#define MC68000		0x0001
#define MC68008		0x0002
#define MC68010		0x0004
#define MC68020		0x0008
#define MC68030		0x0010
#define MC68040		0x0020
#define CPU(m)		((m) & 0x3f)

#define MC68851		0x8000
#define PMMU(m)		((m) & MC68851)
#define MC68881		0x4000
#define MC68882		0x2000
#define FPU(m)		((m) & (MC68881 | MC68882))

#define FIRSTPASS	1
#define LASTPASS	2
#define DEBUGPASS	3
#define DCLABELSPASS	4

#define INCONSISTENT	1
#define CONSISTENT	2

#if CHAR_BIT == 8
#if UCHAR_MAX == 255U
typedef unsigned char	u8bit_t;
#endif
#else
typedef no_8_bit_type	u8bit_t;
#endif
#if USHRT_MAX == 65535U
typedef unsigned short	u16bit_t;
#else
typedef no_16_bit_type	u16bit_t;
#endif
#if UINT_MAX == 4294967295U
typedef unsigned int	u32bit_t;
typedef int		s32bit_t;
#else
typedef no_32_bit_type	u32bit_t;
#endif

typedef u16bit_t	word_t;
typedef s32bit_t	addr_t;

extern FILE	*infp;
extern FILE	*outfp;
extern char	*cc[];
extern char	*bitd[];
extern char	*bitf[];
extern char	buf1[];
extern char	buf2[];
extern char	buf3[];

extern int	pass;
extern int	valid;

extern addr_t	pc;
extern addr_t	ppc;
extern addr_t	initialpc;
extern int	chip;
extern int	lower;
extern int	minlen;
extern int	onepass;
extern int	sp;
extern int	odd;
extern int	linkfallthrough;
extern size_t	slenprint;
#ifndef NOBAD
extern int	dobad;
#endif
extern char	*afile;
extern char	*bfile;
extern char	*ffile;
extern char	*ifile;
extern char	*jfile;
extern char	*nfile;
extern char	*nsfile;

extern jmp_buf	jmp;
extern char	*sfile;

extern long	curoffset;
extern short	flags;
extern addr_t	required[3];
extern int	pcrelative;

struct inst {
	short	size;
	short	flags;
	addr_t	*required;
	short	labelnum;
};

extern struct inst	*insts;
extern addr_t		maxoffset;

/*
 * The lowest 2 bits of flags hold the number of dependencies
 * so we start with 4 below.
 */
#define ISBRA	0x0004
#define ISBSR	0x0008
#define ISBRcc	0x0010
#define ISDBcc	0x0020
#define ISRTS	0x0040
#define ISJMP	0x0080
#define ISJSR	0x0100
#define ISLINK	0x0200
#define ISUNLK	0x0400
#define ISLABEL	0x0800
#define ISGOOD	0x1000
#define ISFPU	0x2000

/*
 * These are overloaded for floating-point constants.
 */
#define L_ISSINGLE	ISBRA
#define L_ISDOUBLE	ISBSR
#define L_ISEXTENDED	ISBRcc
#define L_ISPACKED	ISDBcc

extern addr_t	nextlabel(addr_t);
extern void	disassemble(void);

struct cp {
	char	*prefix;
	void	(*gen)(word_t);
	char	*(*cc)(unsigned int);
};

extern struct cp	coproc[];

/* pgen.c functions */
extern void	pgen(word_t);
extern char	*pcc(unsigned);

/* fgen.c functions */
extern void	fgen(word_t);
extern char	*fcc(unsigned);
extern int	flis2type(int);
extern int	ftype2lis(int);
extern size_t	fsizeof(int);

/* inst2.c functions */
extern void	bit_dynamic(word_t);
extern void	bit_static(word_t);
extern void	biti_reg(const char *, int, const char *);
extern void	biti_size(const char *, word_t);
extern void	cmp2_chk2(word_t);
extern void	movep(word_t);
extern void	cas(word_t);
extern void	cas2(word_t);
extern void	moves(word_t);
extern void	move(word_t, int);
extern void	misc_size(const char *, word_t);
extern void	misc_ea(const char *, word_t, int);
extern void	chk(word_t);
extern void	lea(word_t);
extern void	link(word_t, int);
extern void	unlk(word_t);
extern void	swap(word_t);
extern void	bkpt(word_t);
extern void	trap(word_t);
extern void	stop_rtd(const char *);
extern void	movec(int);
extern void	ext(word_t);
extern void	movereg(word_t, const char *, int);
extern void	moveusp(word_t, int);
extern void	movem(word_t, int);
extern void	dbcc(word_t);
extern void	trapcc(word_t);
extern void	scc(word_t);
extern void	pack_unpk(const char *, word_t);
extern void	addq_subq(word_t);
extern void	op1(const char *, word_t);
extern void	op2(const char *, word_t);
extern void	op2long(const char *, word_t);
extern void	opa(const char *, word_t);
extern void	opx(const char *, word_t, int, int);
extern void	exg(word_t, char, char);
extern void	bitfield(word_t);
extern void	getshiftname(char *, int, int);
extern void	shift(word_t);
extern void	cptrapcc(struct cp *, word_t);
extern void	cpdbcc(struct cp *, word_t);
extern void	cpbcc(struct cp *, word_t);
extern void	cpscc(struct cp *, word_t);
extern void	cpsave(const char *, word_t);
extern void	cprestore(const char *, word_t);

/* inst1.c functions */
extern void	bit_movep_immediate(word_t);
extern void	movebyte(word_t);
extern void	movelong(word_t);
extern void	moveword(word_t);
extern void	misc(word_t);
extern void	addq_subq_scc_dbcc_trapcc(word_t);
extern int	bcc_bsr(word_t);
extern void	moveq(word_t);
extern void	or_div_sbcd(word_t);
extern void	sub_subx(word_t);
extern void	aline(word_t);
extern void	cmp_eor(word_t);
extern void	and_mul_abcd_exg(word_t);
extern void	add_addx(word_t);
extern void	shift_rotate_bitfield(word_t);
extern void	coprocessor(word_t);
extern void	fline(word_t);
extern void	unimplemented(word_t);

/* utils.c functions */
extern int	immsprintf(char *, long);
extern long	signextend(long, int);
extern int	nextword(word_t *);
extern long	getval(int, int *);
extern char	*Areg(int);
extern void	Areg2(char [], char, int);
extern int	fponit(u32bit_t *, int, char *);
extern int	getea(char *, word_t, word_t, int);
extern void	instprint(int, const char *, ...);
extern char	*regbyte(char *, unsigned char, char *, int);
extern void	revbits(unsigned long *, size_t);

#endif /* DIS_H */
