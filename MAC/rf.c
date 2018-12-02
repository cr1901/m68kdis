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
#include <sys/types.h>
#include <netinet/in.h>

#if CHAR_BIT == 8
#if UCHAR_MAX == 255U
typedef unsigned char	u8bit_t;
#endif
#else
typedef no_8_bit_type	u8bit_t;	/* error */
#endif
#if USHRT_MAX == 65535U
typedef unsigned short	u16bit_t;
typedef short		s16bit_t;
#else
typedef no_16_bit_type	u16bit_t;	/* error */
#endif
#if UINT_MAX == 4294967295U
typedef unsigned int	u32bit_t;
typedef int		s32bit_t;
#else
typedef no_32_bit_type	u32bit_t;	/* error */
#endif

struct header {
	u32bit_t	data_offset;	/* offset from beginning
					   of resource file to resource data */
	u32bit_t	map_offset;	/* offset from beginning
					   of resource file to resource map */
	u32bit_t	data_length;	/* Length of resource data */
	u32bit_t	map_length;	/* Length of resource map */
};
#define HEADERSIZE	16

struct map {
	/*
	 * First there are 22 bytes of zeros:
	 *
	 * 16 bytes: reserved for copy of resource header
	 *  4 bytes: reserved for handle to next resource map to be searched
	 *  2 bytes: reserved for file reference number
	 */
	char		header[16];
	char		handle[4];
	char		reference_number[2];
	u16bit_t	attributes;		/* resource file attributes */
	u16bit_t	typelist_offset;	/* offset from beginning of
						   resource map to type list */
	u16bit_t	namelist_offset;	/* offset from beginning of
						   resource map to resource
						   name list */
};
#define MAPSIZE		28

/*
 * Resource file attributes
 */
#define MAP_READ_ONLY	0x80	/* set if file is read-only */
#define MAP_COMPACT	0x40	/* set to compact file on update */
#define MAP_CHANGED	0x20	/* set to write map on update */

struct type {
	char		type[4];	/* resource type */
	u16bit_t	nresources;	/* number of resources of this type
					   in the map (NOT minus 1) */
	u16bit_t	offset;		/* offset from beginning of type list
					   to reference list for resources
					   of this type */
};
#define TYPESIZE	8

struct resource {
	u16bit_t	ID;		/* resource ID */
	s16bit_t	name_length_offset;
					/* offset from beginning of resource
					   name list to length of resource name
					   or -1 if none */
	char		attributes;	/* resource attributes */
	u32bit_t	data_length_offset;
					/* offset from beginning of resource
					   data to length of data for this
					   resource */
	/* NOTE: length_offset is really only 24 bits */
};
#define RESOURCESIZE	12

/*
 * Resource attributes
 */
#define RES_SYS_HEAP	0x40	/* set if read into system heap */
#define RES_PURGEABLE	0x20	/* set if purgeable */
#define RES_LOCKED	0x10	/* set if locked */
#define RES_PROTECTED	0x08	/* set if protected */
#define RES_PRELOAD	0x04	/* set if to be preloaded */
#define RES_CHANGED	0x02	/* set if to be written to resource file */

/*
 * #defines specific to this application
 */
#define FREAD(nbytes, s, fp)	if (fread(buf, 1, nbytes, fp) != nbytes) \
					error("read from " s, fp)
#define FREADM(nbytes, s, fp)	if (fread(mbuf, 1, nbytes, fp) != nbytes) \
					error("read from " s, fp)
#define FSEEK(offset, s, fp)	if (fseek(fp, offset, SEEK_SET) == -1) \
					error("seek to " s, fp)

#define CVT32(a, index)		a = ntohl(*(u32bit_t *)&buf[index])
#define CVT16(a, index)		a = ntohs(*(u16bit_t *)&buf[index])

void
od(unsigned char *buf, size_t length)
{
	size_t		nlines = (length + 15) / 16;
	size_t		line;
	size_t		left = length;
	short		charnum;

	for (line = 0; line < nlines; line++) {
		for (charnum = 0; charnum < 16 && left; charnum++, left--) {
			if (charnum && (charnum % 2) == 0)
				putchar(' ');
			printf("%02x", buf[length - left]);
		}
		putchar('\n');
	}
}

void
pstr(const unsigned char *s, int length)
{
	while (length--) {
		if (isprint(*s))
			putchar(*s);
		else
			printf("\\%0o", *s);
		s++;
	}
}

void
error(char *s, FILE *fp)
{
	if (fp && ferror(fp))
		perror(s);
	else
		fprintf(stderr, "Bad file format: %s\n", s);

	exit(1);
}

void
printrf(FILE *fp)
{
	unsigned char	buf[BUFSIZ];
	int		i;
	int		j;
	long		typelist_offset;
	long		namelist_offset;
	long		offset;

	struct header	header;
	char		system_use[112];
	char		app_data[128];
	struct map	map;
	u16bit_t	ntypes;
	struct type	type;
	struct resource	resource;
	u8bit_t		name_length;
	u32bit_t	data_length;
	unsigned char	*mbuf = NULL;
	long		type_offset;
	long		resource_offset;

	/*
	 * Read the header.
	 */
	FREAD(HEADERSIZE, "header", fp);
	CVT32(header.data_offset, 0);
	CVT32(header.map_offset, 4);
	CVT32(header.data_length, 8);
	CVT32(header.map_length, 12);

	/*
	 * The header is followed by 112 bytes reserved
	 * for system use and then 128 bytes available
	 * for application data.
	 */
	if (fread(system_use, 1, sizeof system_use, fp) != sizeof system_use)
		error("system_use", fp);
	if (fread(app_data, 1, sizeof app_data, fp) != sizeof app_data)
		error("app_data", fp);

	/*
	 * Read the resource map.
	 */
	FSEEK(header.map_offset, "map", fp);
	FREAD(MAPSIZE, "map", fp);
	CVT16(map.attributes, 22);
	CVT16(map.typelist_offset, 24);
	typelist_offset = header.map_offset + map.typelist_offset;
	CVT16(map.namelist_offset, 26);
	namelist_offset = header.map_offset + map.namelist_offset;

	printf("Resource file attributes:");
	if (map.attributes == 0)
		printf(" none");
	else {
#define DOATTR(attr, string)	if (map.attributes & attr) { \
					printf(" " string); \
					map.attributes &= ~attr; \
				}
		DOATTR(MAP_READ_ONLY, "read_only")
		DOATTR(MAP_COMPACT, "compact")
		DOATTR(MAP_CHANGED, "changed")
#undef DOATTR
	}
	if (map.attributes)
		printf(" UNKNOWN");
	printf("\n\n");

	/*
	 * The type list contains
	 *
	 * 2 bytes: number of resource types in the map minus 1
	 * 
	 * followed immediately by the types.
	 */
	FSEEK(typelist_offset, "type list", fp);
	FREAD(2, "type list", fp);
	CVT16(ntypes, 0);
	ntypes++;

	/*
	 * Read the types.
	 */
	type_offset = typelist_offset + 2;
	for (i = 0; i < ntypes; i++, type_offset += TYPESIZE) {
		FSEEK(type_offset, "type", fp);
		FREAD(TYPESIZE, "type", fp);
		strncpy(type.type, buf, sizeof type.type);
		CVT16(type.nresources, 4);
		type.nresources++;
		CVT16(type.offset, 6);

		if (i)
			putchar('\n');
		printf("Resource type: \"%4.4s\"\n", type.type);
		printf("\n");

		/*
		 * Read the resources.
		 */
		resource_offset = typelist_offset + type.offset;
		for (j = 0; j < type.nresources; j++,
		  resource_offset += RESOURCESIZE) {
			FSEEK(resource_offset, "resource", fp);
			FREAD(RESOURCESIZE, "resource", fp);
			CVT16(resource.ID, 0);
			CVT16(resource.name_length_offset, 2);
			resource.attributes = buf[4];
			CVT32(resource.data_length_offset, 4);
			resource.data_length_offset &= 0x00ffffff;

			if (j)
				putchar('\n');
			printf("ID %d\n", resource.ID);
			printf("Attributes:");
			if (resource.attributes == 0)
				printf(" none");
			else {
#define DOATTR(attr, string)	if (resource.attributes & attr) { \
					printf(" " string); \
					resource.attributes &= ~attr; \
				}
				DOATTR(RES_SYS_HEAP, "sys_head")
				DOATTR(RES_PURGEABLE, "purgeable")
				DOATTR(RES_LOCKED, "locked")
				DOATTR(RES_PROTECTED, "protected")
				DOATTR(RES_PRELOAD, "preload")
				DOATTR(RES_CHANGED, "changed")
#undef DOATTR
			}
			if (resource.attributes)
				printf(" UNKNOWN");
			putchar('\n');

			if (resource.name_length_offset != -1) {
				/*
				 * Read resource name.
				 */
				FSEEK(namelist_offset
				  + resource.name_length_offset,
				  "resource name", fp);
				FREAD(1, "resource name length", fp);
				name_length = buf[0];
				FREAD(name_length, "actual resource name", fp);
				printf("Name length: %d\n", name_length);
				printf("Name: \"");
				pstr(buf, name_length);
				printf("\"\n");
			} else
				printf("No name\n");
	
			/*
			 * Read resource data.
			 */
			FSEEK(header.data_offset + resource.data_length_offset,
			  "resource data", fp);
			FREAD(4, "resource data length", fp);
			CVT32(data_length, 0);
			if (data_length
			  && (mbuf = realloc(mbuf, data_length)) == NULL) {
				perror("realloc");
				exit(1);
			}
			if (data_length)
				printf("Bytes %lx to %lx\n",
				  (long)(header.data_offset
				  + resource.data_length_offset) + 4,
				  (long)(data_length + header.data_offset
				  + resource.data_length_offset + 4 - 1));
			FREADM(data_length, "actual resource data", fp);
			od(mbuf, data_length);
		}
	}
}

int
main(int argc, char **argv)
{
	FILE	*fp;

	if (--argc) {
		if ((fp = fopen(argv[1], "r")) == NULL) {
			perror(argv[1]);
			exit(1);
		}
	} else {
		fprintf(stderr, "Usage: %s filename\n", argv[0]);
		exit(2);
	}

	printrf(fp);

	exit(0);
}
