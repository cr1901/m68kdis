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
 * This file contains macros that test whether mode-register combinations
 * match commonly used categories of addressing modes.
 */

#define ISDATA(m)	((m) == 0)
#define ISDIRECT(m)	((m) == 1)
#define ISINDIRECT(m)	((m) == 2)
#define ISPOST(m)	((m) == 3)
#define ISPRE(m)	((m) == 4)
#define ISDISP(m)	((m) == 5)
#define ISINDEX(m)	((m) == 6)
#define ISSHORT(m,r)	((m) == 7 && (r) == 0)
#define ISLONG(m,r)	((m) == 7 && (r) == 1)
#define ISPCDISP(m,r)	((m) == 7 && (r) == 2)
#define ISPCINDEX(m,r)	((m) == 7 && (r) == 3)
#define ISIMM(m,r)	((m) == 7 && (r) == 4)

/* alterable data */
#define ISADEA(m,r)	(ISDATA(m) | ISINDIRECT(m) | ISPOST(m) | ISPRE(m) \
			  | ISDISP(m) | ISINDEX(m) | ISSHORT(m,r) | ISLONG(m,r))

/* data minus immediate */
#define ISDEAlessIMM(m,r)	(ISADEA(m,r) | ISPCDISP(m,r) | ISPCINDEX(m,r))

/* data */
#define ISDEA(m,r)	(ISDEAlessIMM(m,r) | ISIMM(m,r))

/* alterable memory */
#define ISAMEA(m,r)	(ISINDIRECT(m) | ISPOST(m) | ISPRE(m) | ISDISP(m) \
			  | ISINDEX(m) | ISSHORT(m,r) | ISLONG(m,r))

/* memory */
#define ISMEA(m,r)	(ISAMEA(m,r) | ISIMM(m,r) | ISPCDISP(m,r) \
			  | ISPCINDEX(m,r))

/* alterable */
#define ISAEA(m,r)	(ISADEA(m,r) | ISDIRECT(m))

/* all */
#define ISALLEA(m,r)	(ISDEA(m,r) | ISDIRECT(m))	

/* alterable control */
#define ISACEA(m,r)	(ISINDIRECT(m) | ISDISP(m) | ISINDEX(m) | ISSHORT(m,r) \
			  | ISLONG(m,r))

/* alterable control plus predecrement */
#define ISACEAplusPRE(m,r)	(ISACEA(m,r) | ISPRE(m))

/* control */
#define ISCEA(m,r)	(ISACEA(m,r) | ISPCDISP(m,r) | ISPCINDEX(m,r))

/* control plus postincrement */
#define ISCEAplusPOST(m,r)	(ISCEA(m,r) | ISPOST(m))

/* alterable data  minus data */
#define ISADEAlessDATA(m,r)	(ISINDIRECT(m) | ISPOST(m) | ISPRE(m) \
			| ISDISP(m) | ISINDEX(m) | ISSHORT(m,r) | ISLONG(m,r))
